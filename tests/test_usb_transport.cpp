#include "rotator_transport.h"
#include "servo_control.h"
#include "usb_line_buffer.h"
#include "rotator_motion_math.h"
#include <cassert>
#include <cmath>
#include <iostream>

unsigned long hostMillis = 100;
HostSerial Serial;
static bool moving=false, healthy=true, reverse=false, alpaca=false, accept=true;
static double mechanical=20, offset=0, target=20, lastRelative=0;
static int moveCalls=0, stopCalls=0;
static int zeroCalls=0;
static const char* error="";
bool isServoMoving() { return moving; }
bool isServoFeedbackHealthy() { return healthy; }
const char* getServoMotionError() { return error; }
double getServoAngle() { return mechanical; }
bool getReverseDirection() { return reverse; }
void setReverseDirection(bool value) { reverse=value; }
void stopServo() { moving=false; ++stopCalls; }
void setZeroPointExact() { mechanical=0; ++zeroCalls; }
double wrap(double v) { v=fmod(v,360); return v<0?v+360:v; }
double rotatorPosition() { return wrap(mechanical+offset); }
double rotatorTarget() { return target; }
void rotatorSetTarget(double v) { target=wrap(v); }
double rotatorSyncOffset() { return offset; }
void rotatorSync(double v) { offset=wrap(v-mechanical);target=v; }
bool alpacaOwnsRotator() { return alpaca; }
bool moveServoByAngle(double v) { ++moveCalls;lastRelative=v;if(accept)moving=true;return accept; }
bool moveServoToAngle(double v) { ++moveCalls;if(accept){mechanical=v;moving=true;}return accept; }
static const char* session="11111111111111111111111111111111";
static JsonDocument request(const char* cmd) {
    JsonDocument q; q["id"]="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";q["cmd"]=cmd;q["session"]=session;return q;
}
static JsonDocument execute(JsonDocument q) { JsonDocument r;executeUsbRotator(q,r);return r; }
static JsonDocument command(const char* cmd) { return execute(request(cmd)); }
static JsonDocument value(const char* cmd, double v) { auto q=request(cmd);q["value"]=v;return execute(q); }
static void ok(const JsonDocument& r) { assert(r["error"].as<int>()==0); }
static void bad(const JsonDocument& r, int n) { assert(r["error"].as<int>()==n); }
int main() {
    double planned;
    assert(RotatorMotion::relativeTarget(0,360,-360,360,planned,0,-1) && planned==360);
    assert(RotatorMotion::relativeTarget(0,270,-360,360,planned,0,-1) && planned==270);
    assert(RotatorMotion::relativeTarget(0,-270,-360,360,planned,0,-1) && planned==-270);
    assert(!RotatorMotion::relativeTarget(350,20,-360,360,planned,-350,-1));
    assert(!RotatorMotion::relativeTarget(0,NAN,-360,360,planned,0,-1));
    assert(RotatorMotion::absoluteTarget(350,10,-360,360,planned,-350,-1) && planned==10);
    assert(RotatorMotion::absoluteTarget(10,350,-360,360,planned,-10,-1) && planned==-10);
    assert(!RotatorMotion::absoluteTarget(0,INFINITY,-360,360,planned,0,-1));
    // Reversing direction must not allow the cable to wind beyond the limit.
    assert(!RotatorMotion::relativeTarget(360,-10,-360,360,planned,-360,1));
    assert(!RotatorMotion::absoluteTarget(360,350,-360,360,planned,-360,1));
    assert(RotatorMotion::relativeTarget(360,-10,-360,360,planned,-360,-1));
    initRotatorTransport();
    ok(command("hello")); assert(!usbOwnsRotator());
    bad(command("status"),1031);
    alpaca=true;bad(command("connect"),1035);alpaca=false;
    moving=true;bad(command("connect"),1035);moving=false;
    ok(command("connect"));assert(usbOwnsRotator());
    auto wrong=request("status");wrong["session"]="22222222222222222222222222222222";bad(execute(wrong),1031);
    wrong["cmd"]="connect";bad(execute(wrong),1035);
    auto malformed=request("move");malformed["value"]="20";bad(execute(malformed),1025);
    bad(value("absolute",360),1025);bad(value("move",361),1025);
    bad(value("move",NAN),1025);bad(value("absolute",INFINITY),1025);
    healthy=false;bad(value("move",5),1280);healthy=true;
    ok(value("sync",100));assert(offset==80&&target==100&&moveCalls==0);
    ok(command("zero"));assert(mechanical==0&&offset==0&&target==0&&zeroCalls==1);
    ok(value("sync",100));assert(offset==100&&target==100&&moveCalls==0);
    ok(value("absolute",110));assert(mechanical==10&&target==110&&moveCalls==1);
    bad(value("move",1),1035);assert(moveCalls==1);
    ok(command("halt"));assert(!moving&&stopCalls==1);
    ok(value("mechanical",350));assert(mechanical==350&&target==90); // Target is sky angle.
    ok(command("halt"));ok(value("move",-5));assert(lastRelative==-5&&target==85);
    ok(command("halt"));accept=false;bad(value("move",1),1280);accept=true;
    auto r=request("reverse");r["value"]=1;bad(execute(r),1025);r["value"]=true;ok(execute(r));assert(reverse);
    error="Motor stalled";auto state=command("status");ok(state);assert(state["value"]["motionError"]==error);
    moving=true;hostMillis+=15001;processUsbRotator();assert(!moving&&!usbOwnsRotator());
    bad(command("status"),1031);
    ok(command("connect"));moving=true;ok(command("disconnect"));assert(!moving&&!usbOwnsRotator());
    // Exercise the actual serial receiver, including fragmentation and corrupt frames.
    Serial.output.clear();
    std::string hello;serializeJson(request("hello"),hello);hello="@MOROTA "+hello+"\n";
    Serial.input.insert(Serial.input.end(),hello.begin(),hello.begin()+20);processUsbRotator();assert(Serial.output.empty());
    Serial.input.insert(Serial.input.end(),hello.begin()+20,hello.end());while(Serial.available())processUsbRotator();
    assert(Serial.output.find("\n@MOROTA {") == 0);
    assert(Serial.output.find("\"protocol\":1")!=std::string::npos);
    Serial.output.clear();std::string overflow(400,'x');overflow+=hello; // Entire overlong line discarded.
    Serial.input.insert(Serial.input.end(),overflow.begin(),overflow.end());while(Serial.available())processUsbRotator();assert(Serial.output.empty());
    Serial.input.insert(Serial.input.end(),hello.begin(),hello.end());while(Serial.available())processUsbRotator();assert(!Serial.output.empty());
    Serial.output.clear();Serial.input.insert(Serial.input.end(),hello.begin(),hello.begin()+20);processUsbRotator();hostMillis+=1001;processUsbRotator();
    Serial.input.insert(Serial.input.end(),hello.begin()+20,hello.end());while(Serial.available())processUsbRotator();assert(Serial.output.empty());
    Serial.input.insert(Serial.input.end(),hello.begin(),hello.end());while(Serial.available())processUsbRotator();assert(!Serial.output.empty());
    std::cout<<"USB transport: session, ownership, validation, Sync, moves, errors, expiry and framing tests passed.\n";
}
