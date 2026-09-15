#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <deque>
#include <ArduinoJson.h>
class String : public std::string {
public:
    using std::string::string;
    String(const std::string& s) : std::string(s) {}
    bool isEmpty() const { return empty(); }
    size_t write(uint8_t c) { push_back((char)c); return 1; }
    size_t write(const uint8_t* p, size_t n) { append((const char*)p, n); return n; }
};
namespace ArduinoJson {
template<> struct Converter<String> {
    static void toJson(const String& s, JsonVariant v) { v.set(static_cast<const std::string&>(s)); }
    static String fromJson(JsonVariantConst v) { return String(v.as<std::string>()); }
    static bool checkJson(JsonVariantConst v) { return v.is<const char*>(); }
};
}
extern unsigned long hostMillis;
inline unsigned long millis() { return hostMillis; }
struct HostSerial {
    std::deque<char> input;
    std::string output;
    int available() { return (int)input.size(); }
    int read() { char c = input.front(); input.pop_front(); return c; }
    void println(const char* s) { output += s; output += '\n'; }
    size_t write(const uint8_t* p, size_t n) { output.append((const char*)p,n); return n; }
};
extern HostSerial Serial;
