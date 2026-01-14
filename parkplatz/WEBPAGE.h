const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>MoMa Rotator Panel</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
        html{font-family:Arial;background:#181818;color:#ededed}
        body{max-width:360px;margin:0 auto;padding:20px;background:rgb(27,90,76);border-radius:12px;box-shadow:0 0 8px #111}
        h2{font-size:2rem;padding:20px 0 10px;margin:0;text-align:center}
        .centered{display:flex;justify-content:center;align-items:center;margin-bottom:12px}
        .section{display:flex;align-items:center;justify-content:center;margin-bottom:8px;gap:8px}
        label{margin:0 8px 0 0;font-size:1.1rem}
        .small-label{font-size:0.85rem;color:#ddd;margin:2px 0 8px;text-align:center}
        .output-field{font-size:1.3rem;background:#e3a71d;padding:12px;border-radius:5px;text-align:center;border:1px solid #870b0b;min-width:100px;margin-bottom:4px}
        input[type="number"]{width:120px;height:28px;font-size:1.1rem;padding:4px 8px;border-radius:4px;border:2px solid #e3eae3;background:rgb(55,137,75);color:#ededed;text-align:center}
        .button{margin:4px;padding:8px 16px;border:0;cursor:pointer;background:#4247b7;color:#fff;border-radius:6px;font-size:1.1rem;outline:0;min-width:90px}
        .button-stop{margin:4px;padding:8px 16px;border:0;cursor:pointer;background:#b74242;color:#fff;border-radius:6px;font-size:1.1rem;outline:0}
        .button:hover{background:#ff494d}
        .button:active{background:#4247b7}
        .divider{border-top:2px solid #555;margin:20px 0;width:100%}
        .section-title{text-align:center;font-size:1.2rem;margin:15px 0 10px;color:#fdc100}
        .checkbox-container{display:flex;align-items:center;margin-left:10px}
        .checkbox-container input{width:20px;height:20px;cursor:pointer}
        .checkbox-container label{margin-left:5px}
        .input-row{display:flex;align-items:center;justify-content:center;gap:8px;margin-bottom:8px}
        .switch{position:relative;display:inline-block;width:50px;height:24px;margin-left:10px}
        .switch input{display:none}
        .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#444;transition:.4s;border-radius:34px}
        .slider:before{position:absolute;content:"";height:20px;width:20px;left:2px;bottom:2px;background-color:#fff;transition:.4s;border-radius:50%}
        input:checked+.slider{background-color:#4287b7}
        input:checked+.slider:before{transform:translateX(26px)}
        .output-line{display:flex;color:#ccc;font-size:0.95em;justify-content:center;margin-top:20px}
    </style>
</head>
<body>
    <h2>Rotator Panel</h2>

    <!-- Position Anzeige -->
    <div class="centered" style="flex-direction:column">
        <label>Position in &deg;</label>
        <div class="output-field" id="virtual-pos">0.0</div>
    </div>

    <!-- Eingabe mit Goto Button und Reverse Checkbox -->
    <div class="input-row">
        <input type="number" id="posInput" min="0" max="359.99" step="0.01" value="0">
        <button class="button" onclick="gotoTarget()">Goto</button>
        <div class="checkbox-container">
            <input type="checkbox" id="reverseCheck">
            <label for="reverseCheck">Reverse</label>
        </div>
    </div>
    <div class="small-label">(0 .. 359.99&deg;)</div>

    <div class="divider"></div>

    <!-- Goto Position -->
    <div class="section-title">Goto Position ...</div>
    
    <!-- Vier Funktionsbuttons -->
    <div class="section">
        <button class="button-stop" onclick="cmd(1,2)">Stop</button>
        <button class="button" onclick="cmd(1,6)">0&deg;</button>
        <button class="button" onclick="cmd(1,1)">90&deg;</button>
        <button class="button" onclick="cmd(1,5)">180&deg;</button>
    </div>

    <div class="divider"></div>

    <!-- Sync Current Position as -->
    <div class="section-title">Sync Current Position as</div>

    <!-- Weitere Funktionen -->
    <div class="section">
        <button class="button" onclick="cmd(1,11)">Middle</button>
        <button class="button" onclick="cmd(1,18)">Zero </button>
    </div>

    <div class="section">
        <label>Speed:</label>
        <button class="button" onclick="cmd(1,7)">+</button>
        <button class="button" onclick="cmd(1,8)">-</button>
    </div>

    <div class="centered">
        <label>OLED Display:</label>
        <label class="switch">
            <input type="checkbox" id="displaySwitch" checked onchange="cmd(1,this.checked?21:20)">
            <span class="slider"></span>
        </label>
    </div>

    <div class="output-line" id="ip">--.--.--.--</div>

<script>
    function cmd(t,i,a=0,b=0,p=0,d=0){
        var x=new XMLHttpRequest();
        x.open("GET",`cmd?inputT=${t}&inputI=${i}&inputA=${a}&inputB=${b}&inputP=${p}&inputD=${d}`,true);
        x.send();
    }

    function gotoTarget(){
        var inputValue = parseFloat(document.getElementById("posInput").value);
        var reverse = document.getElementById("reverseCheck").checked;
        
        // Validierung: 0 bis 359.99° (Getriebe-Bereich)
        if(isNaN(inputValue) || inputValue < 0 || inputValue > 359.99){
            alert('Bitte Wert zwischen 0 und 359.99 eingeben');
            return;
        }
        
        // Winkelberechnung: W > 180° → Abs(180 - W)
        var effectiveAngle = inputValue;
        if(inputValue > 180){
            effectiveAngle = Math.abs(180.0 - inputValue);
        }
        
        // Übersetzung 1:2 - Getriebe dreht halb so schnell wie Motor
        // Max: 180° (Getriebe) → 360° (Motor) → eine Umdrehung
        const GEAR_RATIO = 2.0;
        var motorDegrees = effectiveAngle * GEAR_RATIO;
        
        // Umrechnung Motor-Grad zu Steps (4096 Steps = 360°, 0-4095)
        var targetSteps = (motorDegrees / 360.0) * 4096;
        if(reverse) {
            targetSteps = -targetSteps;
        }
        
        var currentSteps = (parseFloat(document.getElementById("virtual-pos").getAttribute('data-steps')) || 0);
        
        console.log('Eingabe:', inputValue, '° → Effektiv:', effectiveAngle, '° → Motor:', motorDegrees, '° → Steps:', targetSteps);
        cmd(1,17,0,0,targetSteps,currentSteps);
    }

    function getData(){
        var x=new XMLHttpRequest();
        x.onreadystatechange=function(){
            if(this.readyState==4&&this.status==200){
                var gearDegrees = parseFloat(this.responseText);
                document.getElementById("virtual-pos").innerHTML=gearDegrees.toFixed(2);
            }
        };
        x.open("GET","position",true);
        x.send();
    }

    function getIP(){
        var x=new XMLHttpRequest();
        x.onreadystatechange=function(){
            if(this.readyState==4&&this.status==200){
                document.getElementById("ip").innerHTML=this.responseText;
            }
        };
        x.open("GET","printip",true);
        x.send();
    }

    // Initialize
    setInterval(getData, 300);
    setInterval(getIP, 1200);
</script>
</body>
</html>
)rawliteral";
