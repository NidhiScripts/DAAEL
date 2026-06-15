#include <WiFi.h>
#include <WebServer.h>

// ESP32 HardwareSerial for connecting to Arduino
#define RXD2 16
#define TXD2 17

const char* ssid = "Suhas";
const char* password = "12345678";

// Replace with your ESP32-CAM's IP address if known. 
// Example: "http://192.168.1.100:81/stream"
String esp32CamStreamUrl = "http://YOUR_ESP32_CAM_IP:81/stream";

WebServer server(80);

// Stores the most recently received valid JSON payload from the Arduino
String latestJsonData = "{\"t\":0,\"h\":0,\"g\":0,\"q\":\"normal\",\"seq\":0,\"node\":\"UNO\"}";

// HTML Content for the Web Dashboard (A placeholder that redirects or shows a message, 
// since the comprehensive dashboard will run as a standalone responsive web application)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" class="dark">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>QoS-Aware Industrial IoT Router Control Center</title>
  
  <!-- CDNs for styles, icons, and charts -->
  <script src="https://cdn.tailwindcss.com"></script>
  <script src="https://cdn.jsdelivr.net/npm/apexcharts"></script>
  <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css" rel="stylesheet">
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800;900&family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
  
  <script>
    tailwind.config = {
      darkMode: 'class',
      theme: {
        extend: {
          fontFamily: {
            sans: ['Outfit', 'sans-serif'],
            mono: ['JetBrains Mono', 'monospace'],
          },
          boxShadow: {
            'glow-emerald': '0 0 15px rgba(16, 185, 129, 0.4)',
            'glow-rose': '0 0 15px rgba(244, 63, 94, 0.5)',
            'glow-cyan': '0 0 15px rgba(6, 182, 212, 0.4)',
          }
        }
      }
    }
  </script>

  <style>
    body {
      background-color: #09090b;
      color: #f4f4f5;
      background-image: 
        radial-gradient(at 0% 0%, rgba(20, 83, 45, 0.1) 0, transparent 50%),
        radial-gradient(at 50% 0%, rgba(8, 51, 68, 0.1) 0, transparent 50%),
        radial-gradient(at 100% 0%, rgba(88, 28, 135, 0.1) 0, transparent 50%);
      background-attachment: fixed;
    }

    .glass-card {
      background: rgba(24, 24, 27, 0.6);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid rgba(63, 63, 70, 0.5);
      box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.4);
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    }
    
    .glass-card:hover {
      border-color: rgba(113, 113, 122, 0.5);
      transform: translateY(-2px);
    }

    /* SCADA industrial scanlines */
    .scanlines {
      position: relative;
      overflow: hidden;
    }
    .scanlines::before {
      content: " ";
      display: block;
      position: absolute;
      top: 0; left: 0; bottom: 0; right: 0;
      background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(0, 255, 0, 0.02), rgba(0, 0, 255, 0.06));
      z-index: 2;
      background-size: 100% 4px, 6px 100%;
      pointer-events: none;
    }

    @keyframes pulse-critical {
      0%, 100% { box-shadow: 0 0 15px 0px rgba(244, 63, 94, 0.6); border-color: rgba(244, 63, 94, 1); }
      50% { box-shadow: 0 0 30px 10px rgba(244, 63, 94, 0.2); border-color: rgba(244, 63, 94, 0.4); }
    }

    .alert-glow {
      animation: pulse-critical 2s infinite ease-in-out;
    }

    /* Custom scrollbars */
    ::-webkit-scrollbar {
      width: 6px;
      height: 6px;
    }
    ::-webkit-scrollbar-track {
      background: rgba(24, 24, 27, 0.3);
    }
    ::-webkit-scrollbar-thumb {
      background: rgba(63, 63, 70, 0.8);
      border-radius: 3px;
    }
    ::-webkit-scrollbar-thumb:hover {
      background: rgba(161, 161, 170, 0.8);
    }

    /* Print styles */
    @media print {
      body {
        background: white !important;
        color: black !important;
      }
      .glass-card {
        background: white !important;
        border: 1px solid #ccc !important;
        box-shadow: none !important;
        color: black !important;
      }
      .no-print {
        display: none !important;
      }
    }
  </style>
</head>
<body class="min-h-screen selection:bg-cyan-500/30 selection:text-cyan-200">

  <!-- LOGIN PANEL OVERLAY -->
  <div id="loginOverlay" class="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-md">
    <div class="glass-card max-w-md w-full p-8 rounded-2xl border border-zinc-800 text-center scanlines shadow-2xl mx-4">
      <div class="flex justify-center mb-4">
        <div class="w-16 h-16 bg-gradient-to-tr from-cyan-500 to-emerald-500 rounded-2xl flex items-center justify-center shadow-lg shadow-cyan-500/20">
          <i class="fa-solid fa-shield-halved text-2xl text-zinc-950"></i>
        </div>
      </div>
      <h2 class="text-2xl font-black tracking-tight text-white mb-2">SCADA ACCESS CONTROL</h2>
      <p class="text-zinc-400 text-sm mb-6">Enter authorized gateway credentials to initialize router monitor.</p>
      
      <div id="loginError" class="hidden text-xs text-rose-500 bg-rose-950/40 border border-rose-900/50 p-2.5 rounded-lg mb-4 text-left">
        <i class="fa-solid fa-triangle-exclamation mr-1.5"></i> Invalid credentials. Use <strong>admin</strong> / <strong>admin123</strong>.
      </div>
      
      <form id="loginForm" class="space-y-4">
        <div class="text-left">
          <label class="text-xs uppercase text-zinc-500 font-bold block mb-1.5">Operator Username</label>
          <div class="relative">
            <input type="text" id="username" value="admin" class="w-full bg-zinc-950/60 border border-zinc-800 rounded-lg py-2 px-3 text-sm focus:outline-none focus:border-cyan-500 transition" required>
            <i class="fa-solid fa-user absolute right-3 top-3 text-zinc-600"></i>
          </div>
        </div>
        <div class="text-left">
          <label class="text-xs uppercase text-zinc-500 font-bold block mb-1.5">Gateway Security Key</label>
          <div class="relative">
            <input type="password" id="password" value="admin123" class="w-full bg-zinc-950/60 border border-zinc-800 rounded-lg py-2 px-3 text-sm focus:outline-none focus:border-cyan-500 transition" required>
            <i class="fa-solid fa-lock absolute right-3 top-3 text-zinc-600"></i>
          </div>
        </div>
        <button type="submit" class="w-full bg-gradient-to-r from-cyan-500 to-emerald-500 text-zinc-950 font-black py-2.5 rounded-lg text-sm hover:from-cyan-400 hover:to-emerald-400 transition transform active:scale-95 flex justify-center items-center gap-2">
          <i class="fa-solid fa-right-to-bracket"></i> AUTHORIZE SESSION
        </button>
      </form>
      <div class="mt-4">
        <button onclick="bypassLogin()" class="text-xs text-zinc-600 hover:text-zinc-400 transition underline">Bypass auth (demo mode)</button>
      </div>
    </div>
  </div>

  <!-- MAIN DASHBOARD CONTENT -->
  <div id="dashboardContent" class="hidden max-w-7xl mx-auto px-4 py-6 md:px-6 relative">
    
    <!-- TOP HEADER -->
    <header class="glass-card rounded-2xl p-4 md:p-6 mb-6 flex flex-col lg:flex-row justify-between items-center gap-4 relative overflow-hidden scanlines">
      <!-- Decorative grid overlay -->
      <div class="absolute inset-0 bg-[linear-gradient(to_right,#ffffff02_1px,transparent_1px),linear-gradient(to_bottom,#ffffff02_1px,transparent_1px)] bg-[size:14px_24px] pointer-events-none"></div>
      
      <div class="flex items-center gap-4 relative z-10 w-full lg:w-auto">
        <div class="p-3 bg-zinc-900 border border-zinc-800 rounded-xl flex items-center justify-center text-cyan-400 shadow-glow-cyan">
          <i class="fa-solid fa-circle-nodes text-2xl animate-pulse"></i>
        </div>
        <div>
          <h1 class="text-xl md:text-2xl font-black text-white tracking-tight flex items-center gap-2">
            QoS-Aware Industrial IoT Router Control Center
          </h1>
          <div class="flex items-center gap-2 mt-1 text-xs text-zinc-400">
            <span class="font-mono bg-zinc-800/80 px-2 py-0.5 rounded border border-zinc-700">v2.1.0-NOC</span>
            <span>&bull;</span>
            <span>Smart Factory Congestion Router Interface</span>
          </div>
        </div>
      </div>
      
      <!-- NOC indicators & config controls -->
      <div class="flex flex-wrap items-center gap-4 w-full lg:w-auto justify-between lg:justify-end relative z-10 text-sm">
        
        <!-- Live vs Simulation Controls -->
        <div class="flex items-center bg-zinc-900 border border-zinc-800 p-1.5 rounded-xl">
          <button id="btnSim" onclick="setMode('sim')" class="px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 bg-gradient-to-r from-zinc-800 to-zinc-700 text-cyan-400 border border-zinc-700">
            <i class="fa-solid fa-server"></i> SIMULATOR
          </button>
          <button id="btnLive" onclick="setMode('live')" class="px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 text-zinc-400 hover:text-white">
            <i class="fa-solid fa-wifi"></i> GATEWAY LIVE
          </button>
        </div>
        
        <!-- Metrics quick stats -->
        <div class="flex items-center gap-4">
          <div class="bg-zinc-900/80 border border-zinc-800 p-2.5 rounded-xl text-center min-w-[90px]">
            <div class="text-[10px] uppercase font-bold text-zinc-500 leading-none mb-1">Network Health</div>
            <div id="netHealth" class="font-mono text-base font-black text-emerald-400">98.4%</div>
          </div>
          <div class="bg-zinc-900/80 border border-zinc-800 p-2.5 rounded-xl text-center min-w-[90px]">
            <div class="text-[10px] uppercase font-bold text-zinc-500 leading-none mb-1">Active Devices</div>
            <div id="activeDevices" class="font-mono text-base font-black text-cyan-400">4 / 4</div>
          </div>
        </div>
        
        <!-- Status indicator -->
        <div class="flex items-center gap-2 pl-2 border-l border-zinc-800">
          <div class="flex flex-col text-right">
            <span id="systemTime" class="font-mono text-xs text-zinc-300">13:57:02</span>
            <span id="systemStatusText" class="text-[10px] font-bold text-emerald-400 uppercase tracking-wider">ONLINE</span>
          </div>
          <div id="systemIndicatorDot" class="w-3 h-3 rounded-full bg-emerald-500 alert-glow animate-pulse"></div>
        </div>

        <!-- Action tools -->
        <div class="flex items-center gap-2">
          <button onclick="toggleSettingsModal()" class="p-2.5 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-xl text-zinc-400 hover:text-white transition" title="Gateway IP Settings">
            <i class="fa-solid fa-gears"></i>
          </button>
          <button onclick="toggleFullscreen()" class="p-2.5 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-xl text-zinc-400 hover:text-white transition" title="Toggle Fullscreen">
            <i class="fa-solid fa-expand"></i>
          </button>
          <button onclick="logout()" class="p-2.5 bg-zinc-900 hover:bg-rose-950/60 border border-zinc-800 hover:border-rose-900/50 rounded-xl text-zinc-400 hover:text-rose-400 transition" title="Lock System">
            <i class="fa-solid fa-power-off"></i>
          </button>
        </div>
      </div>
    </header>

    <!-- SETTINGS IP CONFIGURATION MODAL -->
    <div id="settingsModal" class="hidden fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm">
      <div class="glass-card max-w-sm w-full p-6 rounded-xl border border-zinc-800 text-left relative mx-4 shadow-xl">
        <h3 class="text-lg font-bold text-white mb-4 flex items-center gap-2"><i class="fa-solid fa-network-wired text-cyan-500"></i> Gateway Config</h3>
        <div class="space-y-4">
          <div>
            <label class="text-xs font-bold text-zinc-400 block mb-1">ESP32 Gateway API Endpoint</label>
            <input type="text" id="gatewayIp" value="192.168.1.100" class="w-full bg-zinc-950 border border-zinc-800 rounded-lg px-3 py-2 text-sm text-white font-mono focus:outline-none focus:border-cyan-500" placeholder="e.g. 192.168.4.1">
            <span class="text-[10px] text-zinc-500 mt-1 block">Specify the IP address assigned to your ESP32 router.</span>
          </div>
          <div>
            <label class="text-xs font-bold text-zinc-400 block mb-1">ESP32-CAM Stream URL</label>
            <input type="text" id="camStreamIp" value="http://192.168.1.101:81/stream" class="w-full bg-zinc-950 border border-zinc-800 rounded-lg px-3 py-2 text-sm text-white font-mono focus:outline-none focus:border-cyan-500">
            <span class="text-[10px] text-zinc-500 mt-1 block">Live stream MJPEG link from the ESP32-CAM module.</span>
          </div>
        </div>
        <div class="flex justify-end gap-2 mt-6">
          <button onclick="toggleSettingsModal()" class="px-4 py-2 bg-zinc-900 border border-zinc-800 rounded-lg text-zinc-400 hover:text-white transition text-xs font-bold">Cancel</button>
          <button onclick="saveSettings()" class="px-4 py-2 bg-gradient-to-r from-cyan-500 to-emerald-500 text-zinc-950 rounded-lg font-bold transition text-xs hover:from-cyan-400 hover:to-emerald-400">Save Changes</button>
        </div>
      </div>
    </div>

    <!-- ROW 1: REAL-TIME SENSOR MONITORING -->
    <section class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-6">
      
      <!-- Temperature Card -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between" id="tempCard">
        <div class="flex justify-between items-start mb-3">
          <div>
            <span class="text-xs font-bold text-zinc-500 uppercase tracking-wider block">DHT11 Temperature</span>
            <h3 class="text-xl font-bold text-zinc-200 mt-0.5">Core Thermo</h3>
          </div>
          <span class="p-2 bg-amber-500/10 border border-amber-500/20 text-amber-500 rounded-lg"><i class="fa-solid fa-temperature-half"></i></span>
        </div>
        <div class="flex items-center justify-between my-2">
          <div class="flex items-baseline gap-1">
            <span id="tempVal" class="text-4xl font-black text-white font-mono tracking-tight">28.5</span>
            <span class="text-zinc-500 text-sm font-semibold">&deg;C</span>
          </div>
          <div class="w-16 h-16 flex items-center justify-center relative">
            <canvas id="tempCircle" class="w-full h-full"></canvas>
            <div class="absolute text-[10px] font-mono text-zinc-400" id="tempPercent">57%</div>
          </div>
        </div>
        <!-- Sparkline Graph placeholder/canvas -->
        <div class="h-14 mt-2" id="tempChartContainer">
          <div id="tempSparkline"></div>
        </div>
        <div class="flex justify-between items-center mt-3 pt-3 border-t border-zinc-800 text-xs">
          <span class="text-zinc-500">Status Alert</span>
          <span id="tempStatus" class="font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-emerald-950/80 text-emerald-400 border border-emerald-900/30">Normal</span>
        </div>
      </div>

      <!-- Humidity Card -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between" id="humCard">
        <div class="flex justify-between items-start mb-3">
          <div>
            <span class="text-xs font-bold text-zinc-500 uppercase tracking-wider block">DHT11 Humidity</span>
            <h3 class="text-xl font-bold text-zinc-200 mt-0.5">Hydration level</h3>
          </div>
          <span class="p-2 bg-sky-500/10 border border-sky-500/20 text-sky-500 rounded-lg"><i class="fa-solid fa-droplet"></i></span>
        </div>
        <div class="flex items-center justify-between my-2">
          <div class="flex items-baseline gap-1">
            <span id="humVal" class="text-4xl font-black text-white font-mono tracking-tight">65</span>
            <span class="text-zinc-500 text-sm font-semibold">% RH</span>
          </div>
          <div class="w-16 h-16 flex items-center justify-center relative">
            <canvas id="humCircle" class="w-full h-full"></canvas>
            <div class="absolute text-[10px] font-mono text-zinc-400" id="humPercent">65%</div>
          </div>
        </div>
        <div class="h-14 mt-2" id="humChartContainer">
          <div id="humSparkline"></div>
        </div>
        <div class="flex justify-between items-center mt-3 pt-3 border-t border-zinc-800 text-xs">
          <span class="text-zinc-500">Sensor health</span>
          <span class="font-bold text-emerald-400"><i class="fa-solid fa-check mr-1"></i> OPTIMAL</span>
        </div>
      </div>

      <!-- MQ-2 Gas Card -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between" id="gasCard">
        <div class="flex justify-between items-start mb-3">
          <div>
            <span class="text-xs font-bold text-zinc-500 uppercase tracking-wider block">MQ-2 Gas Sensor</span>
            <h3 class="text-xl font-bold text-zinc-200 mt-0.5">Air Quality / Safety</h3>
          </div>
          <span class="p-2 bg-rose-500/10 border border-rose-500/20 text-rose-500 rounded-lg"><i class="fa-solid fa-fire-flame-curved"></i></span>
        </div>
        <div class="flex items-center justify-between my-2">
          <div class="flex items-baseline gap-1">
            <span id="gasVal" class="text-4xl font-black text-white font-mono tracking-tight">210</span>
            <span class="text-zinc-500 text-sm font-semibold">ppm</span>
          </div>
          <div class="flex flex-col text-right">
            <span id="gasStatus" class="font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-emerald-950/80 text-emerald-400 border border-emerald-900/30">SAFE ZONE</span>
            <span class="text-[9px] text-zinc-500 font-mono mt-1">LMT: 400 ppm</span>
          </div>
        </div>
        <!-- Real-time Level Progress bar with ranges -->
        <div class="mt-2.5">
          <div class="flex justify-between text-[9px] text-zinc-500 font-mono mb-1">
            <span>Normal (0)</span>
            <span>Warning (300)</span>
            <span>Danger (400+)</span>
          </div>
          <div class="w-full h-3 bg-zinc-950 border border-zinc-800 rounded-full overflow-hidden p-[2px]">
            <div id="gasProgress" class="h-full bg-gradient-to-r from-emerald-500 via-amber-500 to-rose-500 rounded-full transition-all duration-300" style="width: 45%;"></div>
          </div>
        </div>
        <div class="h-10 mt-2" id="gasChartContainer">
          <div id="gasSparkline"></div>
        </div>
      </div>

      <!-- ESP32-CAM Feed Card -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between relative overflow-hidden" id="camCard">
        <div class="flex justify-between items-start mb-3">
          <div>
            <span class="text-xs font-bold text-zinc-500 uppercase tracking-wider block">ESP32-CAM Feed</span>
            <h3 class="text-xl font-bold text-zinc-200 mt-0.5">Visual Sentinel</h3>
          </div>
          <span class="p-2 bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 rounded-lg" id="camIcon"><i class="fa-solid fa-video"></i></span>
        </div>
        
        <!-- Video Stream Box -->
        <div class="relative w-full aspect-video md:aspect-auto md:h-28 bg-zinc-950 border border-zinc-800 rounded-xl overflow-hidden flex items-center justify-center">
          
          <!-- Mock stream simulation view -->
          <div id="mockStreamContainer" class="absolute inset-0 z-10 flex flex-col justify-between p-2">
            <!-- Detection overlay with mockup bounding box -->
            <div id="camBoundingBox" class="absolute border-2 border-cyan-400 text-cyan-400 text-[8px] font-bold px-1 py-0.5 rounded bg-cyan-950/40 pointer-events-none transition-all duration-1000" style="top: 25%; left: 30%; width: 40%; height: 50%;">
              [VALVE: SECURE]
            </div>
            
            <div class="flex justify-between items-center w-full relative z-20 text-[9px] font-mono bg-black/60 backdrop-blur-sm px-1.5 py-0.5 rounded border border-zinc-800">
              <span class="text-emerald-400 animate-pulse"><i class="fa-solid fa-circle text-[6px] mr-1"></i>STREAM</span>
              <span id="camFps">24.2 FPS</span>
            </div>
            <div class="flex justify-between items-end w-full relative z-20 text-[8px] font-mono text-zinc-400 bg-black/60 backdrop-blur-sm p-1 rounded">
              <span>FOV: ZONE_A_1</span>
              <span>AI DETECT: OK</span>
            </div>
          </div>
          
          <!-- Actual Live Stream frame (only active in LIVE mode when URL is set) -->
          <iframe id="liveStreamIframe" class="w-full h-full border-none hidden" title="ESP32-CAM Live Feed" src=""></iframe>
          
          <!-- Stream error placeholder -->
          <div id="streamOfflineMessage" class="hidden absolute inset-0 z-20 flex flex-col items-center justify-center bg-zinc-950 text-center p-4">
            <i class="fa-solid fa-video-slash text-2xl text-zinc-600 mb-2"></i>
            <span class="text-xs text-zinc-500">Camera Stream Offline</span>
            <span class="text-[9px] text-zinc-600 font-mono mt-1" id="camStreamUrlText"></span>
          </div>
        </div>
        
        <div class="flex justify-between items-center mt-3 pt-3 border-t border-zinc-800 text-xs">
          <span class="text-zinc-500 font-bold">Status</span>
          <span id="camStatus" class="font-bold text-cyan-400 text-xs flex items-center gap-1.5"><i class="fa-solid fa-circle text-[8px] text-cyan-400 animate-ping"></i> CONNECTED</span>
        </div>
      </div>
      
    </section>

    <!-- ROW 2: QoS AND NETWORK MANAGEMENT -->
    <section class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
      
      <!-- Dynamic Bandwidth Allocation -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between relative">
        <div>
          <div class="flex justify-between items-start mb-4">
            <div>
              <h3 class="text-lg font-black text-white flex items-center gap-2">
                <i class="fa-solid fa-chart-pie text-cyan-400"></i> Dynamic Bandwidth Allocation
              </h3>
              <p class="text-xs text-zinc-500 mt-1">Real-time prioritization of heterogeneous sensor channels under dynamic link constraints.</p>
            </div>
            <div class="text-right">
              <span class="text-xs font-bold text-cyan-400 font-mono uppercase bg-cyan-950/80 border border-cyan-900/40 px-2 py-0.5 rounded">Dynamic allocation</span>
            </div>
          </div>
          
          <div class="grid grid-cols-1 md:grid-cols-2 gap-6 items-center my-4">
            <!-- Pie Chart -->
            <div class="flex justify-center">
              <div id="bandwidthPieChart" class="w-full max-w-[240px]"></div>
            </div>
            
            <!-- Bandwidth Details -->
            <div class="space-y-3 font-mono text-xs">
              <div class="flex justify-between border-b border-zinc-800 pb-1.5">
                <span class="text-zinc-500">Total Capacity:</span>
                <span class="text-white font-bold" id="bwTotal">1024 Kbps</span>
              </div>
              <div class="flex justify-between border-b border-zinc-800 pb-1.5">
                <span class="text-zinc-500">Allocated Bandwidth:</span>
                <span class="text-cyan-400 font-bold" id="bwUsed">440 Kbps</span>
              </div>
              <div class="flex justify-between border-b border-zinc-800 pb-1.5">
                <span class="text-zinc-500">Free Reservation:</span>
                <span class="text-emerald-400 font-bold" id="bwFree">584 Kbps</span>
              </div>
              <div class="space-y-2 pt-2 text-[11px]">
                <div class="flex items-center justify-between text-rose-400">
                  <div class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded bg-rose-500"></span> Critical (Gas Emergencies)</div>
                  <span id="bwCritical" class="font-bold">120 Kbps</span>
                </div>
                <div class="flex items-center justify-between text-cyan-400">
                  <div class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded bg-cyan-400"></span> High (ESP32-CAM stream)</div>
                  <span id="bwHigh" class="font-bold">250 Kbps</span>
                </div>
                <div class="flex items-center justify-between text-amber-500">
                  <div class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded bg-amber-500"></span> Medium (Temperature data)</div>
                  <span id="bwMedium" class="font-bold">50 Kbps</span>
                </div>
                <div class="flex items-center justify-between text-emerald-400">
                  <div class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded bg-emerald-400"></span> Low (Humidity reports)</div>
                  <span id="bwLow" class="font-bold">20 Kbps</span>
                </div>
              </div>
            </div>
          </div>
        </div>
        <div class="p-3 bg-zinc-950/60 border border-zinc-800/80 rounded-xl text-xs text-zinc-400 flex items-center gap-2">
          <i class="fa-solid fa-bolt text-amber-500"></i>
          <span>Congestion response level is auto-tuned based on system priority rules.</span>
        </div>
      </div>

      <!-- QoS Packet Scheduler Visualizer -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between">
        <div>
          <div class="flex justify-between items-start mb-4">
            <div>
              <h3 class="text-lg font-black text-white flex items-center gap-2">
                <i class="fa-solid fa-list-check text-violet-400"></i> QoS Packet Scheduler
              </h3>
              <p class="text-xs text-zinc-500 mt-1">Visualizing prioritization queues and scheduling decisions inside the router buffer.</p>
            </div>
            <div class="text-right">
              <span class="text-xs text-zinc-400 font-mono">Queue Length: <span id="queueLength" class="text-white font-bold">2</span></span>
            </div>
          </div>

          <!-- Queue animation widget -->
          <div class="bg-zinc-950 border border-zinc-800 rounded-xl p-4 my-3 font-mono relative overflow-hidden h-40 flex flex-col justify-between">
            <div class="flex items-center justify-between text-[10px] text-zinc-500 border-b border-zinc-800 pb-1.5 mb-2">
              <span>Incoming Buffer</span>
              <span class="text-violet-400 font-bold"><i class="fa-solid fa-microchip mr-1"></i> Scheduler Engine (WFQ)</span>
              <span>Transmit (RF)</span>
            </div>

            <!-- Visual Lane for Queue -->
            <div class="relative flex-1 flex items-center justify-between gap-2">
              
              <!-- Queued packet slots (Left) -->
              <div id="packetQueueList" class="flex gap-2 items-center flex-1 h-14 border-r border-dashed border-zinc-800 pr-4 overflow-hidden relative">
                <!-- Javascript will inject packets sliding here -->
              </div>
              
              <!-- Scheduler processor block (Center) -->
              <div class="w-24 h-16 border-2 border-violet-500 bg-violet-950/20 rounded-xl flex flex-col items-center justify-center text-center relative px-1">
                <div class="absolute -top-2 bg-violet-600 text-[8px] font-bold text-white px-1.5 py-0.5 rounded leading-none">PROCESSOR</div>
                <span id="scheduledPacketName" class="text-xs font-bold text-white leading-tight">--</span>
                <span id="scheduledPacketPriority" class="text-[8px] uppercase tracking-wider text-zinc-400 mt-1">IDLE</span>
              </div>
              
              <!-- Transmit output (Right) -->
              <div class="w-16 h-14 border border-zinc-800 bg-zinc-900 rounded-lg flex flex-col items-center justify-center gap-1 relative overflow-hidden">
                <div class="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-ping absolute top-1 right-1"></div>
                <span class="text-[10px] text-zinc-500 uppercase leading-none">Link Out</span>
                <i class="fa-solid fa-arrow-right-from-bracket text-zinc-400"></i>
              </div>

            </div>

            <!-- Stats overlay -->
            <div class="flex justify-between text-[10px] text-zinc-400 mt-2 border-t border-zinc-800 pt-1.5">
              <span>Decision: <strong id="schedDecision" class="text-cyan-400">Transmit Highest QoS First</strong></span>
              <span>Avg Queue Wait: <strong id="avgWaitTime" class="text-white">12 ms</strong></span>
            </div>
          </div>
        </div>
        <div class="p-3 bg-zinc-950/60 border border-zinc-800/80 rounded-xl text-xs text-zinc-400 flex items-center gap-2">
          <i class="fa-solid fa-circle-info text-cyan-400"></i>
          <span>Packet transmission order: MQ-2 Gas (Critical) > ESP32-CAM (High) > DHT11 Temp (Medium) > DHT11 Humid (Low).</span>
        </div>
      </div>

    </section>

    <!-- ROW 3: KNAPSACK ALGORITHM VISUALIZATION -->
    <section class="mb-6">
      <div class="glass-card rounded-2xl p-5 md:p-6">
        <div class="flex flex-col lg:flex-row justify-between lg:items-start gap-4 mb-4">
          <div>
            <h3 class="text-lg font-black text-white flex items-center gap-2">
              <i class="fa-solid fa-toolbox text-amber-500"></i> Intelligent Packet Selection Engine (0/1 Knapsack optimization)
            </h3>
            <p class="text-xs text-zinc-500 mt-1">Under strict bandwidth bottlenecks, the gateway selects the most valuable packet subset that fits current capacity limits.</p>
          </div>
          
          <!-- Capacity controls and optimization scores -->
          <div class="flex flex-wrap items-center gap-4 text-xs font-mono">
            <div class="flex items-center gap-2 bg-zinc-900 border border-zinc-800 p-2 rounded-xl">
              <span class="text-zinc-500">Capacity limit:</span>
              <input type="range" id="knapsackCapacity" min="200" max="1200" value="800" oninput="solveKnapsack()" class="w-24 accent-amber-500">
              <span id="knapsackCapacityLabel" class="text-amber-500 font-bold">800 Bytes</span>
            </div>
            
            <div class="bg-zinc-900 border border-zinc-800 px-3 py-2 rounded-xl text-center">
              <span class="text-[9px] uppercase text-zinc-500 block">Total Utility Value</span>
              <span id="knapsackScore" class="text-base font-black text-emerald-400">220 Pts</span>
            </div>
          </div>
        </div>

        <div class="grid grid-cols-1 lg:grid-cols-3 gap-6 items-stretch">
          <!-- Table of incoming packets and selection -->
          <div class="lg:col-span-2 bg-zinc-950/60 border border-zinc-850 rounded-xl overflow-hidden">
            <div class="overflow-x-auto">
              <table class="w-full text-left font-mono text-xs">
                <thead>
                  <tr class="bg-zinc-900/80 border-b border-zinc-800 text-zinc-400 uppercase text-[10px] tracking-wider">
                    <th class="p-3">Seq</th>
                    <th class="p-3">Source Device</th>
                    <th class="p-3">Priority Class</th>
                    <th class="p-3 text-right">Size (Weight)</th>
                    <th class="p-3 text-right">Value (Criticality)</th>
                    <th class="p-3 text-center">Engine Selection</th>
                  </tr>
                </thead>
                <tbody id="knapsackTableBody">
                  <!-- Javascript updates items -->
                </tbody>
              </table>
            </div>
          </div>

          <!-- Live explanation card and animation panel -->
          <div class="bg-zinc-900/60 border border-zinc-800/80 rounded-xl p-5 flex flex-col justify-between">
            <div class="space-y-4">
              <div class="flex items-center gap-2 text-xs font-bold text-amber-500 uppercase tracking-wider">
                <i class="fa-solid fa-bolt animate-bounce"></i> Real-time Decision Matrix
              </div>
              <p id="knapsackExplanation" class="text-xs text-zinc-400 leading-relaxed">
                Critical packets selected first to maximize network utility under bandwidth constraints.
              </p>
              
              <div class="border-t border-zinc-800 pt-3 space-y-2.5 font-mono text-xs">
                <div class="flex justify-between">
                  <span class="text-zinc-500">Packets in Buffer:</span>
                  <span id="ksIncomingCount" class="text-white font-bold">5</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-zinc-500">Size Selected:</span>
                  <span id="ksSelectedWeight" class="text-cyan-400 font-bold">512 Bytes</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-zinc-500">Payload Dropped:</span>
                  <span id="ksDroppedWeight" class="text-rose-500 font-bold">256 Bytes</span>
                </div>
              </div>
            </div>

            <!-- Small progress indicator -->
            <div class="mt-4 pt-3 border-t border-zinc-800">
              <div class="flex justify-between text-[10px] text-zinc-500 font-mono mb-1">
                <span>Optimizer Efficiency</span>
                <span id="ksEfficiency">88%</span>
              </div>
              <div class="w-full h-2 bg-zinc-950 rounded-full overflow-hidden">
                <div id="ksEfficiencyBar" class="h-full bg-emerald-500" style="width: 88%;"></div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>

    <!-- ROW 4: CONGESTION MANAGEMENT -->
    <section class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
      
      <!-- Congestion Detection Panel -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between">
        <div>
          <div class="flex justify-between items-start mb-4">
            <div>
              <h3 class="text-lg font-black text-white flex items-center gap-2">
                <i class="fa-solid fa-triangle-exclamation text-rose-500"></i> Congestion Detection Panel
              </h3>
              <p class="text-xs text-zinc-500 mt-1">Monitors critical physical network links to dynamically adjust traffic priorities.</p>
            </div>
            <span id="congestionStatusBadge" class="text-[10px] font-mono font-bold uppercase px-2 py-0.5 rounded bg-emerald-950/80 text-emerald-400 border border-emerald-900/30">
              NORMAL LOAD
            </span>
          </div>

          <div class="grid grid-cols-2 md:grid-cols-3 gap-4 my-2 text-center">
            
            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Network Load</span>
              <span id="netLoadText" class="text-lg font-black font-mono text-cyan-400">42.5%</span>
              <div class="w-full h-1 bg-zinc-900 rounded-full mt-2 overflow-hidden">
                <div id="netLoadBar" class="h-full bg-cyan-400" style="width: 42.5%;"></div>
              </div>
            </div>
            
            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Packet Loss Rate</span>
              <span id="packetLossText" class="text-lg font-black font-mono text-emerald-400">0.05%</span>
              <div class="w-full h-1 bg-zinc-900 rounded-full mt-2 overflow-hidden">
                <div id="packetLossBar" class="h-full bg-emerald-500" style="width: 1%;"></div>
              </div>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Throughput</span>
              <span id="throughputText" class="text-lg font-black font-mono text-white">440 kbps</span>
              <div class="w-full h-1 bg-zinc-900 rounded-full mt-2 overflow-hidden">
                <div id="throughputBar" class="h-full bg-zinc-300" style="width: 44%;"></div>
              </div>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Latency</span>
              <span id="latencyText" class="text-lg font-black font-mono text-white">12 ms</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Jitter</span>
              <span id="jitterText" class="text-lg font-black font-mono text-white">2.5 ms</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">QoS Efficiency</span>
              <span id="qosEfficiencyText" class="text-lg font-black font-mono text-emerald-400">100%</span>
            </div>

          </div>
        </div>
        
        <div class="flex items-center justify-between text-xs text-zinc-400 mt-3 pt-3 border-t border-zinc-800">
          <span>Alert Trigger: <strong class="text-zinc-300">Gas &gt; 400 or Latency &gt; 80ms</strong></span>
          <span class="flex items-center gap-1"><span class="w-2 h-2 rounded-full bg-emerald-500"></span> Links optimal</span>
        </div>
      </div>

      <!-- Packet Analytics -->
      <div class="glass-card rounded-2xl p-5 flex flex-col justify-between">
        <div>
          <div class="flex justify-between items-start mb-4">
            <div>
              <h3 class="text-lg font-black text-white flex items-center gap-2">
                <i class="fa-solid fa-chart-line text-emerald-500"></i> Packet Analytics Counters
              </h3>
              <p class="text-xs text-zinc-500 mt-1">Real-time statistics of routed vs discarded payloads across active IoT routing interfaces.</p>
            </div>
            <button onclick="resetPacketAnalytics()" class="text-xs font-bold text-zinc-500 hover:text-zinc-300 transition flex items-center gap-1.5"><i class="fa-solid fa-rotate-left"></i> Reset</button>
          </div>

          <div class="grid grid-cols-2 md:grid-cols-3 gap-4 my-2 text-center font-mono">
            
            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Packets Received</span>
              <span id="pktsRecv" class="text-2xl font-black text-cyan-400">1,248</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Packets Sent</span>
              <span id="pktsSent" class="text-2xl font-black text-white">1,236</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Critical Packets</span>
              <span id="pktsCritical" class="text-2xl font-black text-rose-500">45</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Dropped Packets</span>
              <span id="pktsDropped" class="text-2xl font-black text-zinc-500">12</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Retransmissions</span>
              <span id="pktsRetrans" class="text-2xl font-black text-amber-500">8</span>
            </div>

            <div class="bg-zinc-950 border border-zinc-850 p-3 rounded-xl">
              <span class="text-[9px] uppercase font-bold text-zinc-500 block mb-1">Success Rate</span>
              <span id="pktSuccessRate" class="text-2xl font-black text-emerald-400">99.03%</span>
            </div>

          </div>
        </div>
        <div class="p-3 bg-zinc-950/60 border border-zinc-800/80 rounded-xl text-xs text-zinc-400 flex items-center gap-2">
          <i class="fa-solid fa-arrows-spin text-cyan-400 animate-spin"></i>
          <span>Success Rate measures total routed critical and high priority packet delivery metrics.</span>
        </div>
      </div>

    </section>

    <!-- ROW 5: EMERGENCY ALERT CENTER -->
    <section class="mb-6">
      <div class="glass-card rounded-2xl p-5 md:p-6">
        <div class="flex justify-between items-center mb-4">
          <div>
            <h3 class="text-lg font-black text-white flex items-center gap-2">
              <i class="fa-solid fa-shield-halved text-rose-500"></i> Emergency Control Center Log
            </h3>
            <p class="text-xs text-zinc-500 mt-1">Visual safety logs detailing alarm statuses, threshold triggers, and network security actions.</p>
          </div>
          <div class="flex items-center gap-2">
            <button onclick="clearAlertLog()" class="px-3 py-1.5 bg-zinc-900 border border-zinc-800 rounded-lg text-xs font-bold text-zinc-400 hover:text-white transition">Clear Logs</button>
            <span class="w-2 h-2 rounded-full bg-emerald-500 alert-glow"></span>
          </div>
        </div>

        <div class="bg-zinc-950/80 border border-zinc-850 rounded-xl overflow-hidden max-h-56 overflow-y-auto">
          <table class="w-full text-left font-mono text-xs">
            <thead>
              <tr class="bg-zinc-900/80 border-b border-zinc-800 text-zinc-500 uppercase text-[9px] tracking-wider">
                <th class="p-3">Timestamp</th>
                <th class="p-3">Severity</th>
                <th class="p-3">Source Node</th>
                <th class="p-3">Alert Alarm Event Message</th>
              </tr>
            </thead>
            <tbody id="alertLogTableBody">
              <!-- Logs will be written dynamically -->
            </tbody>
          </table>
        </div>
      </div>
    </section>

    <!-- ROW 6: SYSTEM PERFORMANCE ANALYTICS -->
    <section class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
      
      <!-- Chart Group 1: Bandwidth & Utilization -->
      <div class="glass-card rounded-2xl p-5">
        <h3 class="text-base font-bold text-white mb-4 flex items-center gap-2">
          <i class="fa-solid fa-network-wired text-cyan-400"></i> Dynamic Bandwidth & Network Utilization Trends
        </h3>
        <div id="bandwidthTrendChart" class="h-64"></div>
      </div>

      <!-- Chart Group 2: Latency & Packet Drops -->
      <div class="glass-card rounded-2xl p-5">
        <h3 class="text-base font-bold text-white mb-4 flex items-center gap-2">
          <i class="fa-solid fa-gauge-high text-rose-500"></i> Link Latency & Packet Drop Analytics
        </h3>
        <div id="latencyDropTrendChart" class="h-64"></div>
      </div>

    </section>

    <!-- ROW 7: DIGITAL TWIN NETWORK VIEW -->
    <section class="mb-6">
      <div class="glass-card rounded-2xl p-5 md:p-6">
        <div class="flex justify-between items-start mb-4">
          <div>
            <h3 class="text-lg font-black text-white flex items-center gap-2">
              <i class="fa-solid fa-cubes text-emerald-400"></i> Digital Twin Router Network Topology
            </h3>
            <p class="text-xs text-zinc-500 mt-1">Animated schema showing live data flow from physical factory floor sensors to the cloud control system.</p>
          </div>
          <span class="text-xs font-mono text-zinc-400 bg-zinc-900 border border-zinc-800 px-2.5 py-1 rounded-lg">
            Active Packets: <span id="twinPacketCount" class="text-cyan-400 font-bold">0</span>
          </span>
        </div>

        <!-- Canvas Visualizer for Digital Twin -->
        <div class="w-full bg-zinc-950 border border-zinc-900 rounded-xl p-4 overflow-hidden relative" style="height: 240px;">
          <!-- Grid overlay -->
          <div class="absolute inset-0 bg-[linear-gradient(to_right,#ffffff01_1px,transparent_1px),linear-gradient(to_bottom,#ffffff01_1px,transparent_1px)] bg-[size:20px_20px]"></div>
          <canvas id="twinCanvas" class="w-full h-full relative z-10"></canvas>
        </div>
      </div>
    </section>

    <!-- BOTTOM EXPORTS & CONTROL BAR -->
    <footer class="flex flex-col md:flex-row justify-between items-center gap-4 py-6 border-t border-zinc-850 text-xs text-zinc-500">
      <div>
        QoS-Aware IoT Router dashboard control center &bull; Live updates via telemetry protocols.
      </div>
      <div class="flex items-center gap-3 no-print">
        <button onclick="exportCSV()" class="px-4 py-2 bg-zinc-900 hover:bg-zinc-850 border border-zinc-800 hover:border-zinc-700 rounded-lg font-bold text-zinc-300 hover:text-white transition flex items-center gap-1.5">
          <i class="fa-solid fa-file-csv text-emerald-400"></i> Export CSV logs
        </button>
        <button onclick="downloadAnalyticsReport()" class="px-4 py-2 bg-zinc-900 hover:bg-zinc-850 border border-zinc-800 hover:border-zinc-700 rounded-lg font-bold text-zinc-300 hover:text-white transition flex items-center gap-1.5">
          <i class="fa-solid fa-file-import text-cyan-400"></i> Save JSON Report
        </button>
        <button onclick="generatePDF()" class="px-4 py-2 bg-gradient-to-r from-cyan-500 to-emerald-500 text-zinc-950 rounded-lg font-black transition flex items-center gap-1.5 hover:from-cyan-400 hover:to-emerald-400">
          <i class="fa-solid fa-file-pdf"></i> Generate PDF Report
        </button>
      </div>
    </footer>

  </div>

  <!-- JAVASCRIPT LOGIC -->
  <script>
    // State Variables
    let loggedIn = false;
    let mode = 'sim'; // 'sim' or 'live'
    let timerId = null;
    let simulationCounter = 0;
    
    // IP Settings
    let gatewayIp = window.location.hostname || "192.168.1.100";
    let camStreamIp = "http://192.168.1.101:81/stream";

    // Sensor Readings History
    let readingsHistory = {
      temp: [26, 26.5, 27, 27.2, 28, 28.5],
      hum: [60, 61, 62, 60.5, 63, 65],
      gas: [120, 140, 150, 180, 200, 210]
    };

    // System Stats
    let stats = {
      recv: 0,
      sent: 0,
      critical: 0,
      dropped: 0,
      retrans: 0,
      load: 35.5,
      loss: 0.05,
      throughput: 320,
      latency: 12,
      jitter: 2.2
    };

    // Knapsack Parameters
    let knapsackCapacity = 800; // bytes
    let incomingPackets = [
      { seq: 101, src: 'MQ2 Gas Sensor', name: 'Gas Level Report', priority: 'Critical', size: 250, val: 100, status: 'Selected' },
      { seq: 102, src: 'ESP32-CAM Stream', name: 'Cam Frame Packet', priority: 'High', size: 450, val: 80, status: 'Selected' },
      { seq: 103, src: 'DHT11 Sensor', name: 'Temperature Reading', priority: 'Medium', size: 150, val: 35, status: 'Selected' },
      { seq: 104, src: 'DHT11 Sensor', name: 'Humidity Reading', priority: 'Low', size: 120, val: 20, status: 'Dropped' },
      { seq: 105, src: 'ESP32 Gateway', name: 'NOC Status Beacon', priority: 'Low', size: 100, val: 10, status: 'Dropped' }
    ];

    // Chart Handles
    let bwPieChartHandle = null;
    let bwTrendChartHandle = null;
    let latencyTrendChartHandle = null;
    
    // Sparkline Handles
    let tempSparklineHandle = null;
    let humSparklineHandle = null;
    let gasSparklineHandle = null;

    // Digital Twin Canvas setup
    let canvas, ctx;
    let nodes = [];
    let particles = [];

    // On Load
    window.onload = function() {
      // Check auth from local storage
      if (localStorage.getItem("scadaAuth") === "true") {
        loginSuccess();
      }
      
      // Load saved settings
      if (localStorage.getItem("scadaGatewayIp")) {
        gatewayIp = localStorage.getItem("scadaGatewayIp");
      }
      document.getElementById("gatewayIp").value = gatewayIp;
      
      if (localStorage.getItem("scadaCamIp")) {
        camStreamIp = localStorage.getItem("scadaCamIp");
        document.getElementById("camStreamIp").value = camStreamIp;
      }

      // Default date & time
      updateDateTime();
      setInterval(updateDateTime, 1000);
    };

    // Auth actions
    document.getElementById("loginForm").addEventListener("submit", function(e) {
      e.preventDefault();
      const user = document.getElementById("username").value;
      const pass = document.getElementById("password").value;
      if (user === "admin" && pass === "admin123") {
        localStorage.setItem("scadaAuth", "true");
        loginSuccess();
      } else {
        document.getElementById("loginError").classList.remove("hidden");
      }
    });

    function bypassLogin() {
      localStorage.setItem("scadaAuth", "true");
      loginSuccess();
    }

    function loginSuccess() {
      document.getElementById("loginOverlay").classList.add("hidden");
      document.getElementById("dashboardContent").classList.remove("hidden");
      
      // Initialize charts and diagrams
      initCharts();
      initDigitalTwin();
      
      // Determine default mode based on protocol
      const initialMode = window.location.protocol === 'file:' ? 'sim' : 'live';
      setMode(initialMode);
      
      // Start Simulator loops
      startLoops();
      
      // Fetch stream URL if live
      if (mode === 'live') {
        fetchStreamUrl();
      }
      
      // Log initialization
      logEvent("INFO", "UNO", "SCADA telemetry portal established. Authorized session initialized.");
    }

    function logout() {
      localStorage.removeItem("scadaAuth");
      location.reload();
    }

    // Toggle Settings
    function toggleSettingsModal() {
      document.getElementById("settingsModal").classList.toggle("hidden");
    }

    function saveSettings() {
      gatewayIp = document.getElementById("gatewayIp").value;
      camStreamIp = document.getElementById("camStreamIp").value;
      localStorage.setItem("scadaGatewayIp", gatewayIp);
      localStorage.setItem("scadaCamIp", camStreamIp);
      toggleSettingsModal();
      logEvent("INFO", "GATEWAY", "Networking settings re-written: Gateway IP is " + gatewayIp);
      
      // If live mode, retrigger camera URL setting
      if (mode === 'live') {
        setupCamFrame();
      }
    }

    // Mode Toggle
    function setMode(newMode) {
      mode = newMode;
      const simBtn = document.getElementById("btnSim");
      const liveBtn = document.getElementById("btnLive");
      
      if (mode === 'sim') {
        simBtn.className = "px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 bg-gradient-to-r from-zinc-800 to-zinc-700 text-cyan-400 border border-zinc-700";
        liveBtn.className = "px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 text-zinc-400 hover:text-white";
        document.getElementById("systemStatusText").innerText = "SIMULATOR ONLINE";
        document.getElementById("systemStatusText").className = "text-[10px] font-bold text-cyan-400 uppercase tracking-wider";
        document.getElementById("systemIndicatorDot").className = "w-3 h-3 rounded-full bg-cyan-500 alert-glow animate-pulse";
        
        // Return stream to mock video
        setupCamFrame();
        logEvent("INFO", "GATEWAY", "Switched to internal NOC simulation environment.");
      } else {
        liveBtn.className = "px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 bg-gradient-to-r from-zinc-800 to-zinc-700 text-emerald-400 border border-zinc-700";
        simBtn.className = "px-3 py-1 rounded-lg text-xs font-bold transition flex items-center gap-1.5 text-zinc-400 hover:text-white";
        document.getElementById("systemStatusText").innerText = "GATEWAY LIVE";
        document.getElementById("systemStatusText").className = "text-[10px] font-bold text-emerald-400 uppercase tracking-wider";
        document.getElementById("systemIndicatorDot").className = "w-3 h-3 rounded-full bg-emerald-500 alert-glow animate-pulse";
        
        setupCamFrame();
        logEvent("WARNING", "GATEWAY", "Attempting hardware connection to network: http://" + gatewayIp);
      }
    }

    function setupCamFrame() {
      const iframe = document.getElementById("liveStreamIframe");
      const mockStream = document.getElementById("mockStreamContainer");
      const errBox = document.getElementById("streamOfflineMessage");
      const camStatus = document.getElementById("camStatus");
      const camIcon = document.getElementById("camIcon");

      if (mode === 'sim') {
        iframe.classList.add("hidden");
        errBox.classList.add("hidden");
        mockStream.classList.remove("hidden");
        camStatus.className = "font-bold text-cyan-400 text-xs flex items-center gap-1.5";
        camStatus.innerHTML = `<i class="fa-solid fa-circle text-[8px] text-cyan-400 animate-ping"></i> CONNECTED`;
        camIcon.className = "p-2 bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 rounded-lg";
      } else {
        // Live Mode
        mockStream.classList.add("hidden");
        iframe.src = camStreamIp;
        iframe.classList.remove("hidden");
        
        // Show status loading or offline check
        camStatus.className = "font-bold text-amber-500 text-xs flex items-center gap-1.5";
        camStatus.innerHTML = `<i class="fa-solid fa-circle text-[8px] text-amber-500 animate-pulse"></i> CONNECTING`;
        
        // Simple mock load check
        setTimeout(() => {
          if (mode === 'live') {
            // Since we're in browser and stream IP is probably offline during testing, 
            // display the offline placeholder
            iframe.classList.add("hidden");
            errBox.classList.remove("hidden");
            document.getElementById("camStreamUrlText").innerText = camStreamIp;
            camStatus.className = "font-bold text-rose-500 text-xs flex items-center gap-1.5";
            camStatus.innerHTML = `<i class="fa-solid fa-circle text-[8px] text-rose-500"></i> OFFLINE`;
            camIcon.className = "p-2 bg-rose-500/10 border border-rose-500/20 text-rose-500 rounded-lg";
            logEvent("WARNING", "ESP32-CAM", "Camera stream was not reachable at URL: " + camStreamIp);
          }
        }, 1500);
      }
    }

    function fetchStreamUrl() {
      const url = window.location.protocol === 'file:' 
        ? `http://${gatewayIp}/api/stream_url` 
        : `/api/stream_url`;
      fetch(url)
        .then(res => res.text())
        .then(streamUrl => {
          if (streamUrl && streamUrl.trim() !== "" && !streamUrl.includes("YOUR_ESP32_CAM_IP")) {
            camStreamIp = streamUrl.trim();
            document.getElementById("camStreamIp").value = camStreamIp;
            if (mode === 'live') {
              setupCamFrame();
            }
          }
        })
        .catch(err => console.warn("Failed to fetch stream URL: ", err));
    }

    // Time update
    function updateDateTime() {
      const date = new Date();
      const timeStr = date.toTimeString().split(' ')[0];
      document.getElementById("systemTime").innerText = timeStr;
    }

    // SCADA Logging
    function logEvent(severity, source, message) {
      const table = document.getElementById("alertLogTableBody");
      const timestamp = new Date().toISOString().slice(11, 19);
      
      let badgeColor = "bg-cyan-950 text-cyan-400 border border-cyan-900";
      if (severity === "WARNING") {
        badgeColor = "bg-amber-950 text-amber-400 border border-amber-900";
      } else if (severity === "CRITICAL" || severity === "EMERGENCY") {
        badgeColor = "bg-rose-950 text-rose-400 border border-rose-900";
      }

      const tr = document.createElement("tr");
      tr.className = "border-b border-zinc-900 hover:bg-zinc-900/50 transition";
      tr.innerHTML = `
        <td class="p-3 text-zinc-400 text-xs">${timestamp}</td>
        <td class="p-3">
          <span class="px-2 py-0.5 rounded-[4px] text-[9px] font-black uppercase ${badgeColor}">${severity}</span>
        </td>
        <td class="p-3 text-zinc-300 font-bold">${source}</td>
        <td class="p-3 text-zinc-200">${message}</td>
      `;
      
      table.prepend(tr);
      
      // Limit logs
      if (table.children.length > 30) {
        table.removeChild(table.lastChild);
      }
    }

    function clearAlertLog() {
      document.getElementById("alertLogTableBody").innerHTML = "";
      logEvent("INFO", "SYSTEM", "NOC alert event history wiped by operator.");
    }

    // Loop timers
    function startLoops() {
      // Run calculations and loops every second
      setInterval(tick, 1000);
      
      // Periodic animations
      animateSchedulerQueue();
    }

    // Main Tick (runs every 1 second)
    function tick() {
      simulationCounter++;
      
      if (mode === 'sim') {
        runSimulationData();
      } else {
        fetchGatewayData();
      }
      
      // Shared math calculations
      solveKnapsack();
      updateDynamicBandwidth();
      updateCongestionCalculations();
      updateNOCCharts();
      drawTwinDiagram();
    }

    // Fetch live hardware data
    function fetchGatewayData() {
      const url = window.location.protocol === 'file:' 
        ? `http://${gatewayIp}/api/data` 
        : `/api/data`;
      fetch(url)
        .then(res => res.json())
        .then(data => {
          // Parse JSON format matching: {"t": 25.5, "h": 60.0, "g": 350, "q": "high", "seq": 12}
          const t = parseFloat(data.t) || 0;
          const h = parseFloat(data.h) || 0;
          const g = parseInt(data.g) || 0;
          const q = data.q || "normal";
          
          updateSensorsUI(t, h, g, q === "critical");
          
          // Increment received counters
          stats.recv++;
          stats.sent++;
          if (q === "critical") {
            stats.critical++;
          }
          
          document.getElementById("activeDevices").innerText = "4 / 4";
        })
        .catch(err => {
          console.warn("Live API connection failed: ", err);
          // Fall back to simulation internally but show error warnings in the log periodically
          if (simulationCounter % 10 === 0) {
            logEvent("CRITICAL", "GATEWAY", "API endpoint unreachable. Running recovery routines...");
          }
          runSimulationData();
        });
    }

    // Run Simulator Calculations
    function runSimulationData() {
      // Simulate slow drift in temperature (26 - 32)
      let currentTemp = parseFloat(document.getElementById("tempVal").innerText);
      currentTemp += (Math.random() - 0.48) * 0.4;
      if (currentTemp > 35) currentTemp = 34.5;
      if (currentTemp < 24) currentTemp = 24.5;
      
      // Simulate humidity drift (55 - 75)
      let currentHum = parseInt(document.getElementById("humVal").innerText);
      currentHum += Math.round((Math.random() - 0.5) * 2);
      if (currentHum > 85) currentHum = 80;
      if (currentHum < 40) currentHum = 45;

      // Gas sensor simulator
      let currentGas = parseInt(document.getElementById("gasVal").innerText);
      
      // Randomly spawn a gas incident every 50 seconds to show dynamic scheduling
      const forceGasAlarm = (simulationCounter % 60 >= 40 && simulationCounter % 60 <= 55);
      
      if (forceGasAlarm) {
        currentGas += Math.round(Math.random() * 80 + 30);
        if (currentGas > 650) currentGas = 620;
      } else {
        currentGas -= Math.round(Math.random() * 30);
        if (currentGas < 120) currentGas = 120 + Math.round(Math.random() * 30);
      }

      const isEmergency = currentGas > 400;
      updateSensorsUI(currentTemp, currentHum, currentGas, isEmergency);
      
      // Mock network packet arrival
      stats.recv += Math.round(Math.random() * 3) + 1;
      stats.sent = stats.recv - stats.dropped;
      if (isEmergency) {
        stats.critical += Math.round(Math.random() * 2) + 1;
      }
    }

    // Update UI for sensors
    function updateSensorsUI(temp, hum, gas, isEmergency) {
      temp = parseFloat(temp.toFixed(1));
      hum = Math.round(hum);
      gas = Math.round(gas);

      // Gas visual alert trigger
      const gasCard = document.getElementById("gasCard");
      const gasValSpan = document.getElementById("gasVal");
      const gasStatusSpan = document.getElementById("gasStatus");
      const gasProgress = document.getElementById("gasProgress");

      // Progress bar percentage
      const gasPct = Math.min((gas / 800) * 100, 100);
      gasProgress.style.width = gasPct + "%";

      if (isEmergency) {
        gasCard.className = "glass-card rounded-2xl p-5 flex flex-col justify-between alert-glow border-rose-500 bg-rose-950/10";
        gasValSpan.className = "text-4xl font-black text-rose-500 font-mono tracking-tight";
        gasStatusSpan.innerText = "CRITICAL LIMIT";
        gasStatusSpan.className = "font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-rose-950/80 text-rose-400 border border-rose-900/40";
        
        if (simulationCounter % 8 === 0) {
          logEvent("EMERGENCY", "MQ-2", `HIGH FLAMMABLE GAS CONCENTRATION DETECTED! Level: ${gas} ppm.`);
        }
      } else if (gas > 280) {
        gasCard.className = "glass-card rounded-2xl p-5 flex flex-col justify-between border-amber-500 bg-amber-950/5";
        gasValSpan.className = "text-4xl font-black text-amber-500 font-mono tracking-tight";
        gasStatusSpan.innerText = "WARNING HIGH";
        gasStatusSpan.className = "font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-amber-950/80 text-amber-400 border border-amber-900/30";
      } else {
        gasCard.className = "glass-card rounded-2xl p-5 flex flex-col justify-between";
        gasValSpan.className = "text-4xl font-black text-white font-mono tracking-tight";
        gasStatusSpan.innerText = "SAFE ZONE";
        gasStatusSpan.className = "font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-emerald-950/80 text-emerald-400 border border-emerald-900/30";
      }

      // Temp alert checks
      const tempStatus = document.getElementById("tempStatus");
      if (temp > 32) {
        tempStatus.className = "font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-amber-950/80 text-amber-400 border border-amber-900/30";
        tempStatus.innerText = "WARNING OVERHEAT";
        if (simulationCounter % 15 === 0) {
          logEvent("WARNING", "DHT11", `Thermal threshold elevated: ${temp}°C.`);
        }
      } else {
        tempStatus.className = "font-bold px-2 py-0.5 rounded text-[10px] tracking-wide uppercase bg-emerald-950/80 text-emerald-400 border border-emerald-900/30";
        tempStatus.innerText = "Normal";
      }

      // Write values
      document.getElementById("tempVal").innerText = temp;
      document.getElementById("humVal").innerText = hum;
      gasValSpan.innerText = gas;
      
      // Update historical arrays
      readingsHistory.temp.push(temp);
      readingsHistory.hum.push(hum);
      readingsHistory.gas.push(gas);
      
      // Crop histories
      if (readingsHistory.temp.length > 60) readingsHistory.temp.shift();
      if (readingsHistory.hum.length > 60) readingsHistory.hum.shift();
      if (readingsHistory.gas.length > 60) readingsHistory.gas.shift();

      // Redraw Sparklines
      tempSparklineHandle.updateSeries([{ data: readingsHistory.temp }]);
      humSparklineHandle.updateSeries([{ data: readingsHistory.hum }]);
      gasSparklineHandle.updateSeries([{ data: readingsHistory.gas }]);

      // Draw gauge circles
      drawRadialIndicator("tempCircle", temp, 50, "#f59e0b");
      document.getElementById("tempPercent").innerText = Math.round((temp/50)*100) + "%";
      drawRadialIndicator("humCircle", hum, 100, "#38bdf8");
      document.getElementById("humPercent").innerText = hum + "%";
    }

    function drawRadialIndicator(canvasId, val, maxVal, color) {
      const c = document.getElementById(canvasId);
      const ctx = c.getContext("2d");
      const dpr = window.devicePixelRatio || 1;
      
      c.width = 64 * dpr;
      c.height = 64 * dpr;
      ctx.scale(dpr, dpr);
      
      ctx.clearRect(0, 0, 64, 64);
      
      // Background ring
      ctx.beginPath();
      ctx.arc(32, 32, 26, 0, 2 * Math.PI);
      ctx.strokeStyle = "rgba(63, 63, 70, 0.3)";
      ctx.lineWidth = 4;
      ctx.stroke();
      
      // Active arc
      const percentage = Math.min(val / maxVal, 1.0);
      ctx.beginPath();
      ctx.arc(32, 32, 26, -0.5 * Math.PI, (-0.5 + 2 * percentage) * Math.PI);
      ctx.strokeStyle = color;
      ctx.lineWidth = 5;
      ctx.lineCap = "round";
      ctx.stroke();
    }

    // Dynamic Bandwidth allocation
    function updateDynamicBandwidth() {
      const gas = parseInt(document.getElementById("gasVal").innerText);
      const isEmergency = gas > 400;
      
      let totalBw = 1024;
      let critVal = 20;
      let highVal = 250;
      let medVal = 50;
      let lowVal = 20;

      if (isEmergency) {
        // Shift bandwidth to priority critical MQ2 packets
        critVal = 480; 
        highVal = 180; // throttled video
        medVal = 30;   // throttled temp
        lowVal = 10;   // throttled hum
        
        // Dropping packets simulation
        stats.dropped += Math.round(Math.random() * 2);
        stats.retrans += Math.round(Math.random() * 1);
      } else if (gas > 280) {
        critVal = 150;
        highVal = 220;
      }
      
      let usedBw = critVal + highVal + medVal + lowVal;
      let freeBw = totalBw - usedBw;

      document.getElementById("bwTotal").innerText = totalBw + " Kbps";
      document.getElementById("bwUsed").innerText = usedBw + " Kbps";
      document.getElementById("bwFree").innerText = freeBw + " Kbps";
      
      document.getElementById("bwCritical").innerText = critVal + " Kbps";
      document.getElementById("bwHigh").innerText = highVal + " Kbps";
      document.getElementById("bwMedium").innerText = medVal + " Kbps";
      document.getElementById("bwLow").innerText = lowVal + " Kbps";

      // Update pie chart
      bwPieChartHandle.updateSeries([critVal, highVal, medVal, lowVal, freeBw]);
      
      stats.load = parseFloat(((usedBw / totalBw) * 100).toFixed(1));
      stats.throughput = usedBw;
    }

    // Congestion metrics calculation
    function updateCongestionCalculations() {
      const gas = parseInt(document.getElementById("gasVal").innerText);
      const isEmergency = gas > 400;
      
      if (isEmergency) {
        stats.latency = Math.round(Math.random() * 25 + 65); // high latency
        stats.jitter = parseFloat((Math.random() * 5 + 8.5).toFixed(1));
        stats.loss = parseFloat((Math.random() * 4 + 2.5).toFixed(2));
        
        document.getElementById("congestionStatusBadge").innerText = "LINK CONGESTION ALERT";
        document.getElementById("congestionStatusBadge").className = "text-[10px] font-mono font-bold uppercase px-2 py-0.5 rounded bg-rose-950/80 text-rose-400 border border-rose-900/40 animate-pulse";
      } else {
        stats.latency = Math.round(Math.random() * 4 + 10);
        stats.jitter = parseFloat((Math.random() * 0.8 + 1.2).toFixed(1));
        stats.loss = parseFloat((Math.random() * 0.05 + 0.02).toFixed(2));
        
        document.getElementById("congestionStatusBadge").innerText = "NORMAL LOAD";
        document.getElementById("congestionStatusBadge").className = "text-[10px] font-mono font-bold uppercase px-2 py-0.5 rounded bg-emerald-950/80 text-emerald-400 border border-emerald-900/30";
      }

      // Compute QoS Efficiency
      // QoS Efficiency = (Critical Packets Delivered / Total Critical Packets) * 100
      let qosEff = 100;
      if (stats.dropped > 0 && stats.critical > 0) {
        const deliveredCrit = Math.max(stats.critical - Math.round(stats.dropped * 0.1), 1);
        qosEff = Math.round((deliveredCrit / stats.critical) * 100);
        if (qosEff > 100) qosEff = 100;
      }
      
      document.getElementById("netLoadText").innerText = stats.load + "%";
      document.getElementById("netLoadBar").style.width = stats.load + "%";
      
      document.getElementById("packetLossText").innerText = stats.loss + "%";
      document.getElementById("packetLossBar").style.width = Math.min(stats.loss * 10, 100) + "%";
      
      document.getElementById("throughputText").innerText = stats.throughput + " kbps";
      document.getElementById("throughputBar").style.width = (stats.throughput / 10.24) + "%";
      
      document.getElementById("latencyText").innerText = stats.latency + " ms";
      document.getElementById("jitterText").innerText = stats.jitter + " ms";
      document.getElementById("qosEfficiencyText").innerText = qosEff + "%";

      // Network Health Score = 100 - (Loss * 10) - (Latency/10)
      let health = 100 - (stats.loss * 8) - (stats.latency / 12);
      health = Math.min(Math.max(health, 24.5), 99.9);
      document.getElementById("netHealth").innerText = health.toFixed(1) + "%";
      
      // Render counts
      document.getElementById("pktsRecv").innerText = stats.recv.toLocaleString();
      document.getElementById("pktsSent").innerText = stats.sent.toLocaleString();
      document.getElementById("pktsCritical").innerText = stats.critical.toLocaleString();
      document.getElementById("pktsDropped").innerText = stats.dropped.toLocaleString();
      document.getElementById("pktsRetrans").innerText = stats.retrans.toLocaleString();
      
      let rate = 100;
      if (stats.recv > 0) {
        rate = (stats.sent / stats.recv) * 100;
      }
      document.getElementById("pktSuccessRate").innerText = rate.toFixed(2) + "%";
    }

    function resetPacketAnalytics() {
      stats.recv = 0;
      stats.sent = 0;
      stats.critical = 0;
      stats.dropped = 0;
      stats.retrans = 0;
      logEvent("INFO", "SYSTEM", "NOC packet analytics counter values reset to zero.");
    }

    // 0/1 Knapsack optimization Selection Algorithm
    // Maximize total utility values given a total packet weight capacity
    function solveKnapsack() {
      const capInput = document.getElementById("knapsackCapacity");
      knapsackCapacity = parseInt(capInput.value);
      document.getElementById("knapsackCapacityLabel").innerText = knapsackCapacity + " Bytes";

      const gas = parseInt(document.getElementById("gasVal").innerText);
      const isEmergency = gas > 400;

      // Adjust incoming packets details based on active sensors states
      incomingPackets = [
        { seq: simulationCounter + 12, src: 'MQ2 Sensor', name: 'Gas Level Emergency', priority: 'Critical', size: 256, val: 120, status: 'Dropped' },
        { seq: simulationCounter + 13, src: 'ESP32-CAM', name: 'Video Frame Stream', priority: 'High', size: 512, val: 80, status: 'Dropped' },
        { seq: simulationCounter + 14, src: 'DHT11 Core', name: 'Temperature Log', priority: 'Medium', size: 128, val: 40, status: 'Dropped' },
        { seq: simulationCounter + 15, src: 'DHT11 Core', name: 'Humidity Log', priority: 'Low', size: 96, val: 20, status: 'Dropped' },
        { seq: simulationCounter + 16, src: 'ESP32 Gateway', name: 'Internal Link Beacon', priority: 'Low', size: 64, val: 10, status: 'Dropped' }
      ];

      // If gas is emergency, MQ2 packets value explodes
      if (isEmergency) {
        incomingPackets[0].val = 240; 
        incomingPackets[0].size = 320; // larger packets due to diagnostic trace dumps
      }

      const N = incomingPackets.length;
      
      // Dynamic Programming for 0/1 Knapsack
      // dp[i][w] holds max value
      let dp = Array(N + 1).fill().map(() => Array(knapsackCapacity + 1).fill(0));
      
      for (let i = 1; i <= N; i++) {
        const item = incomingPackets[i - 1];
        for (let w = 0; w <= knapsackCapacity; w++) {
          if (item.size <= w) {
            dp[i][w] = Math.max(dp[i - 1][w], dp[i - 1][w - item.size] + item.val);
          } else {
            dp[i][w] = dp[i - 1][w];
          }
        }
      }

      // Backtracking to find selected items
      let w = knapsackCapacity;
      let totalSelectedWeight = 0;
      let totalSelectedValue = dp[N][knapsackCapacity];
      
      for (let i = N; i > 0; i--) {
        if (dp[i][w] !== dp[i - 1][w]) {
          incomingPackets[i - 1].status = 'Selected';
          totalSelectedWeight += incomingPackets[i - 1].size;
          w -= incomingPackets[i - 1].size;
        } else {
          incomingPackets[i - 1].status = 'Dropped';
        }
      }

      // Render table
      const tbody = document.getElementById("knapsackTableBody");
      tbody.innerHTML = "";
      
      let droppedWeight = 0;
      let incomingCount = N;

      incomingPackets.forEach(item => {
        let statusBadge = "";
        let rowClass = "";
        
        if (item.status === 'Selected') {
          statusBadge = `<span class="px-2 py-0.5 rounded text-[10px] font-black uppercase bg-emerald-950 text-emerald-400 border border-emerald-900"><i class="fa-solid fa-square-check mr-1"></i> SELECT</span>`;
          rowClass = "bg-emerald-950/10 border-l-2 border-l-emerald-500";
        } else {
          statusBadge = `<span class="px-2 py-0.5 rounded text-[10px] font-black uppercase bg-rose-950/60 text-rose-500 border border-rose-900/40"><i class="fa-solid fa-rectangle-xmark mr-1"></i> DROP</span>`;
          rowClass = "bg-rose-950/5 text-zinc-500 opacity-60";
          droppedWeight += item.size;
        }

        let pColor = "text-zinc-400";
        if (item.priority === 'Critical') pColor = "text-rose-500 font-bold";
        else if (item.priority === 'High') pColor = "text-cyan-400";
        else if (item.priority === 'Medium') pColor = "text-amber-500";

        const tr = document.createElement("tr");
        tr.className = `border-b border-zinc-900 transition ${rowClass}`;
        tr.innerHTML = `
          <td class="p-3 font-mono text-zinc-400">#${item.seq}</td>
          <td class="p-3 font-bold text-zinc-200">${item.src}</td>
          <td class="p-3 ${pColor}">${item.priority}</td>
          <td class="p-3 text-right font-mono">${item.size} B</td>
          <td class="p-3 text-right font-mono">${item.val}</td>
          <td class="p-3 text-center">${statusBadge}</td>
        `;
        tbody.appendChild(tr);
      });

      // Write parameters
      document.getElementById("knapsackScore").innerText = totalSelectedValue + " Pts";
      document.getElementById("ksIncomingCount").innerText = incomingCount;
      document.getElementById("ksSelectedWeight").innerText = totalSelectedWeight + " B";
      document.getElementById("ksDroppedWeight").innerText = droppedWeight + " B";

      // Live Explanation update
      const explanationText = document.getElementById("knapsackExplanation");
      if (isEmergency) {
        explanationText.innerHTML = `<span class="text-rose-400 font-bold"><i class="fa-solid fa-circle-exclamation mr-1 animate-pulse"></i> EMERGENCY CONSTRAINTS ACTIVATED:</span> The Knapsack engine prioritized <strong>Gas Level Emergency (valued at 240)</strong>. Throttling applied to lower priority nodes to avoid buffer overload.`;
      } else {
        explanationText.innerHTML = `<span class="text-emerald-400 font-bold"><i class="fa-solid fa-check-double mr-1"></i> CAPACITY OPTIMIZATION ACTIVE:</span> Selecting optimal packet packets. Critical sensor logs fit easily inside the current ${knapsackCapacity}B allocation limit.`;
      }

      // Efficiency calculation
      const efficiency = Math.round((totalSelectedValue / (incomingPackets.reduce((a, b) => a + b.val, 0) || 1)) * 100);
      document.getElementById("ksEfficiency").innerText = efficiency + "%";
      document.getElementById("ksEfficiencyBar").style.width = efficiency + "%";
    }

    // QoS Queue scheduler animator
    let queuePackets = [];
    function animateSchedulerQueue() {
      const queueList = document.getElementById("packetQueueList");
      
      setInterval(() => {
        // Randomly generate new packets into queue
        const srcTypes = [
          { name: 'GAS', pri: 'CRITICAL', color: 'bg-rose-500 shadow-glow-rose', text: 'text-rose-500' },
          { name: 'CAM', pri: 'HIGH', color: 'bg-cyan-400 shadow-glow-cyan', text: 'text-cyan-400' },
          { name: 'TEMP', pri: 'MEDIUM', color: 'bg-amber-500', text: 'text-amber-500' },
          { name: 'HUMID', pri: 'LOW', color: 'bg-emerald-400', text: 'text-emerald-400' }
        ];

        const gas = parseInt(document.getElementById("gasVal").innerText);
        let spawnedType;

        if (gas > 400 && Math.random() < 0.7) {
          spawnedType = srcTypes[0]; // MQ2 Gas
        } else {
          spawnedType = srcTypes[Math.floor(Math.random() * srcTypes.length)];
        }

        const newPkt = {
          id: Math.round(Math.random() * 900) + 100,
          name: spawnedType.name,
          priority: spawnedType.pri,
          color: spawnedType.color,
          text: spawnedType.text
        };

        queuePackets.push(newPkt);
        if (queuePackets.length > 5) {
          // Drop tail to simulate queue buffer limit
          queuePackets.shift();
          stats.dropped++;
        }

        // Render queue in HTML
        queueList.innerHTML = "";
        queuePackets.forEach((p, idx) => {
          const div = document.createElement("div");
          div.className = `w-12 h-10 border border-zinc-700/80 rounded flex flex-col justify-center items-center font-mono text-[9px] relative transition-all duration-500 ${p.color} text-zinc-950 font-bold`;
          div.innerHTML = `
            <span>${p.name}</span>
            <span>#${p.id}</span>
          `;
          queueList.appendChild(div);
        });

        document.getElementById("queueLength").innerText = queuePackets.length;

        // Process first queue packet (Head)
        if (queuePackets.length > 0) {
          // Sort queue packets if scheduler is running QoS prioritization
          queuePackets.sort((a, b) => {
            const priOrder = { 'CRITICAL': 1, 'HIGH': 2, 'MEDIUM': 3, 'LOW': 4 };
            return priOrder[a.priority] - priOrder[b.priority];
          });
          
          const activePkt = queuePackets.shift();
          
          // Write to processor display
          document.getElementById("scheduledPacketName").innerText = activePkt.name + " (#" + activePkt.id + ")";
          document.getElementById("scheduledPacketPriority").innerText = activePkt.priority;
          document.getElementById("scheduledPacketPriority").className = `text-[8px] uppercase tracking-wider ${activePkt.text} mt-1 font-black`;
          
          // Animate particle flow inside digital twin representing this packet type
          spawnTopologyParticle(activePkt.priority);
        } else {
          document.getElementById("scheduledPacketName").innerText = "--";
          document.getElementById("scheduledPacketPriority").innerText = "IDLE";
          document.getElementById("scheduledPacketPriority").className = "text-[8px] uppercase tracking-wider text-zinc-400 mt-1";
        }

      }, 1200);
    }

    // Real-time chart setup using ApexCharts
    function initCharts() {
      // Sparkline charts
      const sparkConfig = (color) => ({
        chart: { type: 'line', height: '100%', sparkline: { enabled: true }, animations: { enabled: false } },
        stroke: { curve: 'smooth', width: 2 },
        colors: [color],
        series: [{ data: [0] }],
        tooltip: { fixed: { enabled: false } }
      });

      tempSparklineHandle = new ApexCharts(document.querySelector("#tempSparkline"), sparkConfig('#f59e0b'));
      tempSparklineHandle.render();

      humSparklineHandle = new ApexCharts(document.querySelector("#humSparkline"), sparkConfig('#38bdf8'));
      humSparklineHandle.render();

      gasSparklineHandle = new ApexCharts(document.querySelector("#gasSparkline"), sparkConfig('#f43f5e'));
      gasSparklineHandle.render();

      // Bandwidth pie chart
      const pieOptions = {
        chart: { type: 'donut', height: 200, foreColor: '#a1a1aa' },
        labels: ['Critical', 'High', 'Medium', 'Low', 'Reserved Free'],
        series: [20, 250, 50, 20, 584],
        colors: ['#f43f5e', '#06b6d4', '#f59e0b', '#10b981', '#27272a'],
        stroke: { show: false },
        legend: { show: false },
        plotOptions: {
          donut: {
            size: '70%',
            labels: {
              show: true,
              total: { show: true, label: 'Allocated', formatter: () => '440K' }
            }
          }
        },
        dataLabels: { enabled: false }
      };
      bwPieChartHandle = new ApexCharts(document.querySelector("#bandwidthPieChart"), pieOptions);
      bwPieChartHandle.render();

      // Performance analytics Trend Charts
      const lineOptions = {
        chart: { type: 'area', height: 240, toolbar: { show: false }, animations: { enabled: true, easing: 'linear', dynamicAnimation: { speed: 800 } } },
        stroke: { curve: 'smooth', width: 2 },
        colors: ['#06b6d4', '#10b981'],
        series: [
          { name: 'Used Bandwidth (Kbps)', data: Array(15).fill(440) },
          { name: 'Network Load (%)', data: Array(15).fill(42) }
        ],
        grid: { borderColor: '#1f1f23' },
        xaxis: { labels: { show: false } },
        yaxis: { labels: { style: { colors: '#71717a' } } },
        legend: { style: { colors: '#a1a1aa' }, position: 'top' },
        theme: { mode: 'dark' },
        fill: {
          type: 'gradient',
          gradient: { shadeIntensity: 1, opacityFrom: 0.35, opacityTo: 0.05 }
        }
      };
      bwTrendChartHandle = new ApexCharts(document.querySelector("#bandwidthTrendChart"), lineOptions);
      bwTrendChartHandle.render();

      const line2Options = {
        chart: { type: 'line', height: 240, toolbar: { show: false } },
        stroke: { curve: 'smooth', width: 2 },
        colors: ['#f43f5e', '#a1a1aa'],
        series: [
          { name: 'Packet Drop Rate (%)', data: Array(15).fill(0) },
          { name: 'Latency (ms)', data: Array(15).fill(12) }
        ],
        grid: { borderColor: '#1f1f23' },
        xaxis: { labels: { show: false } },
        yaxis: { labels: { style: { colors: '#71717a' } } },
        legend: { style: { colors: '#a1a1aa' }, position: 'top' },
        theme: { mode: 'dark' }
      };
      latencyTrendChartHandle = new ApexCharts(document.querySelector("#latencyDropTrendChart"), line2Options);
      latencyTrendChartHandle.render();
    }

    // Update charts dynamically over time
    function updateNOCCharts() {
      // Bandwidth trend updates
      const bwSeries = bwTrendChartHandle.w.config.series;
      let bwData1 = bwSeries[0].data;
      let bwData2 = bwSeries[1].data;
      
      bwData1.push(stats.throughput);
      bwData2.push(stats.load);
      
      if (bwData1.length > 20) {
        bwData1.shift();
        bwData2.shift();
      }
      bwTrendChartHandle.updateSeries([
        { data: bwData1 },
        { data: bwData2 }
      ]);

      // Latency updates
      const latSeries = latencyTrendChartHandle.w.config.series;
      let latData1 = latSeries[0].data;
      let latData2 = latSeries[1].data;

      latData1.push(stats.loss);
      latData2.push(stats.latency);

      if (latData1.length > 20) {
        latData1.shift();
        latData2.shift();
      }
      latencyTrendChartHandle.updateSeries([
        { data: latData1 },
        { data: latData2 }
      ]);
    }

    // Digital Twin Topology Diagram
    function initDigitalTwin() {
      canvas = document.getElementById("twinCanvas");
      ctx = canvas.getContext("2d");
      
      // Node structure
      nodes = [
        { id: 'sensors', name: 'Sensors', desc: 'DHT11 / MQ-2', x: 80, y: 110, icon: '\uf2db' },
        { id: 'uno', name: 'Arduino Uno', desc: 'Edge Node', x: 260, y: 110, icon: '\uf0e8' },
        { id: 'esp32', name: 'ESP32 Gateway', desc: 'API Router', x: 460, y: 110, icon: '\uf530' },
        { id: 'rpi', name: 'Raspberry Pi', desc: 'Edge Router', x: 660, y: 110, icon: '\uf233' },
        { id: 'cloud', name: 'Cloud Dashboard', desc: 'SCADA Host', x: 840, y: 110, icon: '\uf0c2' }
      ];
    }

    function spawnTopologyParticle(priority) {
      let color = "#10b981"; // Low (green)
      let speed = 2;
      
      if (priority === 'CRITICAL') {
        color = "#f43f5e"; // Red
        speed = 5;
      } else if (priority === 'HIGH') {
        color = "#06b6d4"; // Cyan
        speed = 3.5;
      } else if (priority === 'MEDIUM') {
        color = "#f59e0b"; // Yellow
        speed = 2.5;
      }

      particles.push({
        nodeIdx: 0,
        progress: 0,
        speed: speed * 0.015,
        color: color,
        size: priority === 'CRITICAL' ? 6 : 4
      });
    }

    function drawTwinDiagram() {
      if (!canvas || !ctx) return;
      
      const width = canvas.clientWidth;
      const height = canvas.clientHeight;
      canvas.width = width;
      canvas.height = height;

      ctx.clearRect(0, 0, width, height);

      // Reposition nodes based on actual width
      const spacing = width / 6;
      nodes.forEach((n, idx) => {
        n.x = spacing * (idx + 1);
        n.y = height / 2;
      });

      // Draw connection wires with neon glows
      ctx.lineWidth = 3;
      ctx.shadowBlur = 0;
      for (let i = 0; i < nodes.length - 1; i++) {
        const start = nodes[i];
        const end = nodes[i + 1];

        // Animate line waves/glows
        const gradient = ctx.createLinearGradient(start.x, start.y, end.x, end.y);
        gradient.addColorStop(0, "rgba(63, 63, 70, 0.6)");
        gradient.addColorStop(0.5, "rgba(6, 182, 212, 0.25)");
        gradient.addColorStop(1, "rgba(63, 63, 70, 0.6)");

        ctx.beginPath();
        ctx.moveTo(start.x, start.y);
        ctx.lineTo(end.x, end.y);
        ctx.strokeStyle = gradient;
        ctx.stroke();
      }

      // Update and draw flowing packets (particles)
      let activeParticles = 0;
      for (let i = particles.length - 1; i >= 0; i--) {
        const p = particles[i];
        p.progress += p.speed;
        
        if (p.progress >= 1.0) {
          p.progress = 0;
          p.nodeIdx++;
        }

        if (p.nodeIdx >= nodes.length - 1) {
          particles.splice(i, 1); // delete when reaches cloud
          continue;
        }

        activeParticles++;

        const startNode = nodes[p.nodeIdx];
        const endNode = nodes[p.nodeIdx + 1];
        
        // Linear interpolation
        const px = startNode.x + (endNode.x - startNode.x) * p.progress;
        const py = startNode.y + (endNode.y - startNode.y) * p.progress;

        // Draw particle
        ctx.beginPath();
        ctx.arc(px, py, p.size, 0, 2 * Math.PI);
        ctx.fillStyle = p.color;
        ctx.shadowBlur = 8;
        ctx.shadowColor = p.color;
        ctx.fill();
        ctx.shadowBlur = 0; // reset
      }

      document.getElementById("twinPacketCount").innerText = activeParticles;

      // Draw Router Nodes
      nodes.forEach(n => {
        // Outer glowing ring
        ctx.beginPath();
        ctx.arc(n.x, n.y, 24, 0, 2 * Math.PI);
        ctx.fillStyle = "rgba(24, 24, 27, 0.9)";
        ctx.strokeStyle = "rgba(63, 63, 70, 0.8)";
        ctx.lineWidth = 1.5;
        ctx.fill();
        ctx.stroke();

        // Small indicator dot inside node
        ctx.beginPath();
        ctx.arc(n.x + 16, n.y - 16, 4, 0, 2 * Math.PI);
        ctx.fillStyle = "#10b981";
        ctx.fill();

        // Draw icon (FontAwesome mapping manually)
        ctx.fillStyle = "#a1a1aa";
        ctx.font = "14px FontAwesome";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(n.icon, n.x, n.y - 2);

        // Labels
        ctx.fillStyle = "#ffffff";
        ctx.font = "bold 10px Outfit";
        ctx.fillText(n.name, n.x, n.y + 36);

        ctx.fillStyle = "#71717a";
        ctx.font = "8px Outfit";
        ctx.fillText(n.desc, n.x, n.y + 46);
      });
    }

    // Export log records as CSV
    function exportCSV() {
      let csvContent = "data:text/csv;charset=utf-8,";
      csvContent += "Timestamp,Severity,Source,Message\n";
      
      const rows = document.querySelectorAll("#alertLogTableBody tr");
      if (rows.length === 0) {
        alert("Alert event log history is currently empty.");
        return;
      }

      rows.forEach(r => {
        const cols = r.querySelectorAll("td");
        const ts = cols[0].innerText;
        const sev = cols[1].innerText.replace(/[\n\r]+/g, '').trim();
        const src = cols[2].innerText;
        const msg = cols[3].innerText.replace(/,/g, ';'); // replace commas with semi-colon
        csvContent += `${ts},${sev},${src},${msg}\n`;
      });

      const encodedUri = encodeURI(csvContent);
      const link = document.createElement("a");
      link.setAttribute("href", encodedUri);
      link.setAttribute("download", `IoT_SCADA_Alert_Log_${new Date().toISOString().slice(0, 10)}.csv`);
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }

    // Save JSON report containing metrics
    function downloadAnalyticsReport() {
      const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify({
        reportDate: new Date().toISOString(),
        gatewayEndpoint: gatewayIp,
        simulationMode: mode === 'sim',
        sensorHistory: readingsHistory,
        packetsReport: stats
      }, null, 2));
      
      const link = document.createElement("a");
      link.setAttribute("href", dataStr);
      link.setAttribute("download", `IoT_NOC_Analytics_Report_${new Date().getTime()}.json`);
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }

    // PDF Printer Report Generator
    function generatePDF() {
      // Trigger window print dialog which uses print CSS parameters for styling
      window.print();
    }

    // Fullscreen support
    function toggleFullscreen() {
      if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen().catch(err => {
          alert(`Error attempting to enable full-screen mode: ${err.message}`);
        });
      } else {
        document.exitFullscreen();
      }
    }
  </script>
</body>
</html>

)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Initialize Serial2 for communicating with Arduino UNO
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.setTimeout(50); // Set a short timeout so reading from serial doesn't block the web server
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi: ");
  Serial.print(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected successfully!");
  Serial.print("Access the IoT Dashboard API at: http://");
  Serial.println(WiFi.localIP());

  // Web Server Routes with CORS headers
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/api/data", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(200, "application/json", latestJsonData);
  });

  server.on("/api/stream_url", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(200, "text/plain", esp32CamStreamUrl);
  });

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Handle routing data from Arduino
  if (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    incoming.trim();
    
    // Validate that we received a JSON object
    if (incoming.startsWith("{") && incoming.endsWith("}")) {
      latestJsonData = incoming;
      
      // Print the routed data for debugging
      Serial.println("Gateway Routed Packet: " + latestJsonData);
    }
  }
}
