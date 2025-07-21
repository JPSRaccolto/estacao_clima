#include <stdio.h> // Biblioteca padrão para entrada e saída (e.g., printf)
#include "pico/stdlib.h" // Biblioteca padrão para a Raspberry Pi Pico
#include "hardware/i2c.h" // Biblioteca para comunicação I2C
#include "aht20.h" // Driver para o sensor AHT20 (temperatura e umidade)
#include "bmp280.h" // Driver para o sensor BMP280 (pressão e temperatura)
#include <math.h> // Biblioteca matemática (e.g., para pow() usado no cálculo de altitude)
#include "pico/cyw43_arch.h" // Biblioteca para o Wi-Fi CYW43 no Pico W
#include "lwip/pbuf.h" // Biblioteca LwIP para buffers de pacotes (usado em rede)
#include "lwip/tcp.h" // Biblioteca LwIP para comunicação TCP (usado em rede)
#include "lwip/netif.h" // Biblioteca LwIP para interfaces de rede (usado em rede)
#include "ws2812.pio.h" // Driver para LEDs WS2812 (NeoPixel) usando PIO
#include "hardware/pwm.h" // Biblioteca para Pulse Width Modulation (PWM)
#include "pico/bootrom.h" // Biblioteca para funções de bootrom (e.g., reset_usb_boot)

// Definições de pinos e parâmetros importantes para hardware e rede Wi-Fi.
#define botaoB 6
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define SEA_LEVEL_PRESSURE 101325.0
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 2
#define I2C_SCL_DISP 3
#define endereco 0x3C
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BOTAO_A 5
#define AZUL 12
#define VERDE 11
#define VERMELHO 13
#define buzzer1 10
#define buzzer2 21
#define PWM_WRAP 4095
#define PWM_CLK_DIV 30.52f
#define WIFI_SSID "xxxx"
#define WIFI_PASSWORD "xxx"

// Variáveis globais para controle de estado, leitura de sensores e limites configuráveis.
bool estado_buzzer = false;
bool buzzer_ativo = false;
volatile bool pagina_alternativa = false;
volatile int pagina_atual = 0;
absolute_time_t ultimo_bip;
absolute_time_t last_interrupt_time = 0;
int bomba, porcentagem, limite_min, limite_max, ADC_VREF;
int ciclos_buzzer = 0;
AHT20_Data data;
int32_t global_pressure;

// Limites e offsets de calibração para os valores dos sensores.
float temp_min = 5.0f, temp_max = 55.0f;
float umid_min = 30.0f, umid_max = 70.0f;
float press_min = 950.0f, press_max = 1050.0f;
float temp_offset = 0.0f;
float umid_offset = 0.0f;
float press_offset = 0.0f;

// Funções para controle dos LEDs WS2812.
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

int i = 0;

// Definições dos padrões de LEDs para exibir números ou estados.
double desenho0[25] = {
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2
};

double desenho1[25] = {
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.2, 0.2, 0.2, 0.0, 0.2,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.2, 0.0, 0.0, 0.0, 0.2
};

double desenho2[25] = {
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0
};

double desenho3[25] = {
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0
};

double desenho4[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0
};

double desenho5[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.2, 0.0, 0.0, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0
};

double desenho6[25] = {
    0.0, 0.2, 0.0, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.0, 0.2, 0.0
};

// Funções para exibir padrões nos LEDs WS2812 com cores específicas.
void num0(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho0[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num1(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho1[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num2(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho2[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num3(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho3[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num4(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho4[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num5(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho5[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num6(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho6[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

// Inicializa um pino GPIO para operar como PWM.
void pwm_init_gpio(uint gpio, uint wrap, float clkdiv) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, wrap);
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_init(slice_num, &config, true);
}

// Calcula a altitude com base na pressão atmosférica.
double calculate_altitude(double pressure) {
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

// Função de callback para interrupções de GPIO (botões).
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();
    // Debounce para evitar múltiplas detecções de um único pressionar de botão.
    if (absolute_time_diff_us(last_interrupt_time, now) < 250000) return;
    last_interrupt_time = now;

    if(gpio == BOTAO_A) {
        // Altera a página atual e a flag de alternância.
        pagina_atual = (pagina_atual + 1) % 4;
        pagina_alternativa = !pagina_alternativa;
    }
    if(gpio == botaoB) {
        // Reinicia o Pico W para o modo bootloader USB.
        reset_usb_boot(0, 0);
    }
}

// Inicializa todos os pinos GPIO, I2C, PIO e PWM necessários.
void inicia_pinos() {
    // Configura botão B com pull-up e interrupção na borda de descida.
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    // Configura botão A com interrupção.
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
   
    // Inicializa comunicação serial para debug.
    stdio_init_all();
    sleep_ms(50);
    
    // Inicializa I2C para os sensores.
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    // Inicializa I2C para os sensores.
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Configura PIO para os LEDs WS2812.
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    
    // Inicializa PWM para os buzzers.
    pwm_init_gpio(buzzer1, PWM_WRAP, PWM_CLK_DIV);
    pwm_set_gpio_level(buzzer1, 0);
    pwm_init_gpio(buzzer2, PWM_WRAP, PWM_CLK_DIV);
    pwm_set_gpio_level(buzzer2, 0);
    
    // Configura os pinos dos LEDs RGB como saída.
    gpio_init(VERDE);
    gpio_set_dir(VERDE, GPIO_OUT);
    gpio_init(AZUL);
    gpio_set_dir(AZUL, GPIO_OUT);
    gpio_init(VERMELHO);
    gpio_set_dir(VERMELHO, GPIO_OUT);
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
}

// Constantes de strings HTML para as páginas do servidor web embarcado.
const char HTML_BODY[] = 
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Sistema de Monitoramento</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}"
"h1{color:#333;text-align:center;}"
"a{display:block;margin:10px 0;padding:15px;background:#007bff;color:white;text-decoration:none;border-radius:5px;text-align:center;}"
"a:hover{background:#0056b3;}"
".config{background:#fff;padding:20px;border-radius:10px;margin:20px 0;}"
".config input{margin:5px;padding:8px;width:100px;}"
".config button{background:#28a745;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;}"
".config button:hover{background:#218838;}"
"</style>"
"</head><body>"
"<h1>Sistema de Monitoramento</h1>"
"<a href=\"/temp\">Ver Temperatura</a>"
"<a href=\"/umid\">Ver Umidade</a>"
"<a href=\"/press\">Ver Pressao</a>"
"<a href=\"/offset\">Calibrar Offset</a>"
"<div class=\"config\">"
"<h3>Configurar Limites</h3>"
"<form id=\"configForm\">"
"<label>Temp Min: <input type=\"number\" id=\"temp_min\" step=\"0.1\"></label>"
"<label>Temp Max: <input type=\"number\" id=\"temp_max\" step=\"0.1\"></label><br>"
"<label>Umid Min: <input type=\"number\" id=\"umid_min\" step=\"0.1\"></label>"
"<label>Umid Max: <input type=\"number\" id=\"umid_max\" step=\"0.1\"></label><br>"
"<label>Press Min: <input type=\"number\" id=\"press_min\" step=\"0.1\"></label>"
"<label>Press Max: <input type=\"number\" id=\"press_max\" step=\"0.1\"></label><br>"
"<button type=\"submit\">Salvar Configurações</button>"
"</form>"
"</div>"
"<script>"
"let minhaPagina = 0;" 
"setInterval(()=>{"
"  fetch('/pagina').then(r=>r.json()).then(d=>{"
"    if(d.pagina !== minhaPagina){"
"      if(d.pagina==0) window.location.href = '/';"
"      if(d.pagina==1) window.location.href = '/temp';"
"      if(d.pagina==2) window.location.href = '/umid';"
"      if(d.pagina==3) window.location.href = '/press';"
"    }"
"  });"
"}, 1000);"
"fetch('/config').then(r=>r.json()).then(d=>{"
"document.getElementById('temp_min').value=d.temp_min;"
"document.getElementById('temp_max').value=d.temp_max;"
"document.getElementById('umid_min').value=d.umid_min;"
"document.getElementById('umid_max').value=d.umid_max;"
"document.getElementById('press_min').value=d.press_min;"
"document.getElementById('press_max').value=d.press_max;"
"});"
"document.getElementById('configForm').addEventListener('submit',function(e){"
"e.preventDefault();"
"let data='temp_min='+document.getElementById('temp_min').value+"
"'&temp_max='+document.getElementById('temp_max').value+"
"'&umid_min='+document.getElementById('umid_min').value+"
"'&umid_max='+document.getElementById('umid_max').value+"
"'&press_min='+document.getElementById('press_min').value+"
"'&press_max='+document.getElementById('press_max').value;"
"fetch('/config',{method:'POST',body:data}).then(r=>r.json()).then(d=>{"
"alert('Configurações salvas!');"
"});"
"});"
"</script>"
"</body></html>";

const char HTML_OFFSET[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<title>Calibracao</title>"
"<style>"
"body{font-family:sans-serif;background:#fdfdfd;margin:20px;color:#222;}"
"h2,h3{margin-bottom:10px;}"
"form p{margin:6px 0;}"
"input{width:80px;padding:4px;}"
"button{padding:4px 8px;margin-right:8px;}"
"</style>"
"</head><body>"
"<h2>Calibracao de Sensores</h2>"
"<p>Valores atuais:</p>"
"<p>Temperatura: <span id=\"temp\">--</span> C</p>"
"<p>Umidade: <span id=\"umid\">--</span>%</p>"
"<p>Pressao: <span id=\"press\">--</span>hPa</p>"
"<hr>"
"<h3>Ajustar Offset</h3>"
"<form id=\"form\">"
"<p>Offset Temperatura: <input type=\"number\" id=\"toff\" step=\"0.1\" value=\"0\"></p>"
"<p>Offset Umidade: <input type=\"number\" id=\"uoff\" step=\"0.1\" value=\"0\"></p>"
"<p>Offset Pressao: <input type=\"number\" id=\"poff\" step=\"0.1\" value=\"0\"></p>"
"<button type=\"submit\">Salvar</button>"
"<button type=\"button\" onclick=\"zerar()\">Zerar Tudo</button>"
"</form>"
"<p><a href=\"/\">Voltar</a></p>"
"<script>"
"let minhaPagina = 4;"
"setInterval(()=>{"
"  fetch('/pagina').then(r=>r.json()).then(d=>{"
"    if(d.pagina !== minhaPagina){"
"      if(d.pagina==0) window.location.href = '/';"
"      if(d.pagina==1) window.location.href = '/temp';"
"      if(d.pagina==2) window.location.href = '/umid';"
"      if(d.pagina==3) window.location.href = '/press';"
"      if(d.pagina==4) window.location.href = '/offset';"
"    }"
"  });"
"}, 1000);"
"function atualizar(){"
"  fetch('/dados').then(r=>r.json()).then(d=>{"
"    document.getElementById('temp').innerText=d.temperatura.toFixed(1);"
"    document.getElementById('umid').innerText=d.umidade.toFixed(1);"
"    document.getElementById('press').innerText=(d.pressao/100).toFixed(1);"
"  });"
"}"
"function zerar(){"
"  document.getElementById('toff').value='0';"
"  document.getElementById('uoff').value='0';"
"  document.getElementById('poff').value='0';"
"  let data='temp_offset=0&umid_offset=0&press_offset=0';"
"  fetch('/setoffset',{method:'POST',"
"    headers:{'Content-Type':'application/x-www-form-urlencoded'},"
"    body:data}).then(r=>r.json()).then(d=>{"
"      alert('Offsets zerados!');"
"    });"
"}"
"document.getElementById('form').onsubmit=function(e){"
"  e.preventDefault();"
"  let data='temp_offset='+encodeURIComponent(document.getElementById('toff').value)+"
"'&umid_offset='+encodeURIComponent(document.getElementById('uoff').value)+"
"'&press_offset='+encodeURIComponent(document.getElementById('poff').value);"
"  fetch('/setoffset',{method:'POST',"
"    headers:{'Content-Type':'application/x-www-form-urlencoded'},"
"    body:data}).then(r=>r.json()).then(d=>{"
"      alert('Salvo!');"
"    }).catch(e=>{ });"
"};"
"fetch('/getoffset').then(r=>r.json()).then(d=>{"
"  document.getElementById('toff').value=d.temp_offset;"
"  document.getElementById('uoff').value=d.umid_offset;"
"  document.getElementById('poff').value=d.press_offset;"
"});"
"setInterval(atualizar,1000);"
"atualizar();"
"</script>"
"</body></html>";

const char HTML_TEMP[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Temperatura</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}"
"h2{color:#333;text-align:center;}"
"#valor{font-size:24px;font-weight:bold;color:#007bff;}"
"canvas{border:1px solid #ccc;margin:20px 0;}"
"a{display:inline-block;margin:10px 5px;padding:10px 15px;background:#007bff;color:white;text-decoration:none;border-radius:5px;}"
"a:hover{background:#0056b3;}"
"</style>"
"</head><body>"
"<h2>Temperatura</h2>"
"<p>Atual: <span id=\"valor\">---</span>°C</p>"
"<canvas id=\"grafico\" width=\"600\" height=\"300\"></canvas>"
"<div><a href=\"/umid\">Umidade</a><a href=\"/press\">Pressao</a><a href=\"/\">Inicio</a></div>"
"<script>"
"let minhaPagina = 1;" // 1 = temperatura
"setInterval(()=>{"
"  fetch('/pagina').then(r=>r.json()).then(d=>{"
"    if(d.pagina !== minhaPagina){"
"      if(d.pagina==0) window.location.href = '/';"
"      if(d.pagina==1) window.location.href = '/temp';"
"      if(d.pagina==2) window.location.href = '/umid';"
"      if(d.pagina==3) window.location.href = '/press';"
"    }"
"  });"
"}, 1000);"
"let dados=[];"
"let tempos=[];"
"let inicioTempo=Date.now();"
"function atualizar(){"
"fetch('/dados').then(r=>r.json()).then(d=>{"
"document.getElementById('valor').innerText=d.temperatura.toFixed(1);"
"let tempoAtual=(Date.now()-inicioTempo)/1000;"
"dados.push(d.temperatura);"
"tempos.push(tempoAtual);"
"if(dados.length>50){dados.shift();tempos.shift();}"
"desenhar();"
"});"
"}"
"function desenhar(){"
"let c=document.getElementById('grafico');"
"let ctx=c.getContext('2d');"
"ctx.clearRect(0,0,c.width,c.height);"
"if(dados.length<2)return;"
"let minTemp=Math.min(...dados)-5;"
"let maxTemp=Math.max(...dados)+5;"
"let minTempo=Math.min(...tempos);"
"let maxTempo=Math.max(...tempos);"
"ctx.strokeStyle='#ccc';"
"ctx.lineWidth=1;"
"for(let i=0;i<=5;i++){"
"let y=c.height-((i/5)*(c.height-40))-20;"
"ctx.beginPath();"
"ctx.moveTo(40,y);"
"ctx.lineTo(c.width-20,y);"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText((minTemp+(maxTemp-minTemp)*i/5).toFixed(1)+'°C',5,y+3);"
"}"
"ctx.beginPath();"
"ctx.strokeStyle='#007bff';"
"ctx.lineWidth=2;"
"for(let i=0;i<dados.length;i++){"
"let x=40+(tempos[i]-minTempo)/(maxTempo-minTempo)*(c.width-60);"
"let y=c.height-20-((dados[i]-minTemp)/(maxTemp-minTemp))*(c.height-40);"
"if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);"
"}"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText('Tempo (s)',c.width/2-30,c.height-5);"
"}"
"setInterval(atualizar,500);"
"atualizar();"
"</script>"
"</body></html>";

const char HTML_UMID[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Umidade</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}"
"h2{color:#333;text-align:center;}"
"#valor{font-size:24px;font-weight:bold;color:#28a745;}"
"canvas{border:1px solid #ccc;margin:20px 0;}"
"a{display:inline-block;margin:10px 5px;padding:10px 15px;background:#28a745;color:white;text-decoration:none;border-radius:5px;}"
"a:hover{background:#218838;}"
"</style>"
"</head><body>"
"<h2>Umidade</h2>"
"<p>Atual: <span id=\"valor\">---</span>%</p>"
"<canvas id=\"grafico\" width=\"600\" height=\"300\"></canvas>"
"<div><a href=\"/temp\">Temperatura</a><a href=\"/press\">Pressao</a><a href=\"/\">Inicio</a></div>"
"<script>"
"let minhaPagina = 2;" 
"setInterval(()=>{"
"  fetch('/pagina').then(r=>r.json()).then(d=>{"
"    if(d.pagina !== minhaPagina){"
"      if(d.pagina==0) window.location.href = '/';"
"      if(d.pagina==1) window.location.href = '/temp';"
"      if(d.pagina==2) window.location.href = '/umid';"
"      if(d.pagina==3) window.location.href = '/press';"
"    }"
"  });"
"}, 1000);"
"let dados=[];"
"let tempos=[];"
"let inicioTempo=Date.now();"
"function atualizar(){"
"fetch('/dados').then(r=>r.json()).then(d=>{"
"document.getElementById('valor').innerText=d.umidade.toFixed(1);"
"let tempoAtual=(Date.now()-inicioTempo)/1000;"
"dados.push(d.umidade);"
"tempos.push(tempoAtual);"
"if(dados.length>50){dados.shift();tempos.shift();}"
"desenhar();"
"});"
"}"
"function desenhar(){"
"let c=document.getElementById('grafico');"
"let ctx=c.getContext('2d');"
"ctx.clearRect(0,0,c.width,c.height);"
"if(dados.length<2)return;"
"let minUmid=Math.min(...dados)-5;"
"let maxUmid=Math.max(...dados)+5;"
"let minTempo=Math.min(...tempos);"
"let maxTempo=Math.max(...tempos);"
"ctx.strokeStyle='#ccc';"
"ctx.lineWidth=1;"
"for(let i=0;i<=5;i++){"
"let y=c.height-((i/5)*(c.height-40))-20;"
"ctx.beginPath();"
"ctx.moveTo(40,y);"
"ctx.lineTo(c.width-20,y);"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText((minUmid+(maxUmid-minUmid)*i/5).toFixed(1)+'%',5,y+3);"
"}"
"ctx.beginPath();"
"ctx.strokeStyle='#28a745';"
"ctx.lineWidth=2;"
"for(let i=0;i<dados.length;i++){"
"let x=40+(tempos[i]-minTempo)/(maxTempo-minTempo)*(c.width-60);"
"let y=c.height-20-((dados[i]-minUmid)/(maxUmid-minUmid))*(c.height-40);"
"if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);"
"}"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText('Tempo (s)',c.width/2-30,c.height-5);"
"}"
"setInterval(atualizar,500);"
"atualizar();"
"</script>"
"</body></html>";

const char HTML_PRESS[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Pressao</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}"
"h2{color:#333;text-align:center;}"
"#valor{font-size:24px;font-weight:bold;color:#dc3545;}"
"canvas{border:1px solid #ccc;margin:20px 0;}"
"a{display:inline-block;margin:10px 5px;padding:10px 15px;background:#dc3545;color:white;text-decoration:none;border-radius:5px;}"
"a:hover{background:#c82333;}"
"</style>"
"</head><body>"
"<h2>Pressao Atmosferica</h2>"
"<p>Atual: <span id=\"valor\">---</span> hPa</p>"
"<canvas id=\"grafico\" width=\"600\" height=\"300\"></canvas>"
"<div><a href=\"/temp\">Temperatura</a><a href=\"/umid\">Umidade</a><a href=\"/\">Inicio</a></div>"
"<script>"
"let minhaPagina = 3;" 
"setInterval(()=>{"
"  fetch('/pagina').then(r=>r.json()).then(d=>{"
"    if(d.pagina !== minhaPagina){"
"      if(d.pagina==0) window.location.href = '/';"
"      if(d.pagina==1) window.location.href = '/temp';"
"      if(d.pagina==2) window.location.href = '/umid';"
"      if(d.pagina==3) window.location.href = '/press';"
"    }"
"  });"
"}, 1000);"
"let dados=[];"
"let tempos=[];"
"let inicioTempo=Date.now();"
"function atualizar(){"
"fetch('/dados').then(r=>r.json()).then(d=>{"
"let p=d.pressao/100;"
"document.getElementById('valor').innerText=p.toFixed(1);"
"let tempoAtual=(Date.now()-inicioTempo)/1000;"
"dados.push(p);"
"tempos.push(tempoAtual);"
"if(dados.length>50){dados.shift();tempos.shift();}"
"desenhar();"
"});"
"}"
"function desenhar(){"
"let c=document.getElementById('grafico');"
"let ctx=c.getContext('2d');"
"ctx.clearRect(0,0,c.width,c.height);"
"if(dados.length<2)return;"
"let minPress=Math.min(...dados)-5;"
"let maxPress=Math.max(...dados)+5;"
"let minTempo=Math.min(...tempos);"
"let maxTempo=Math.max(...tempos);"
"ctx.strokeStyle='#ccc';"
"ctx.lineWidth=1;"
"for(let i=0;i<=5;i++){"
"let y=c.height-((i/5)*(c.height-40))-20;"
"ctx.beginPath();"
"ctx.moveTo(40,y);"
"ctx.lineTo(c.width-20,y);"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText((minPress+(maxPress-minPress)*i/5).toFixed(1)+' hPa',5,y+3);"
"}"
"ctx.beginPath();"
"ctx.strokeStyle='#dc3545';"
"ctx.lineWidth=2;"
"for(let i=0;i<dados.length;i++){"
"let x=40+(tempos[i]-minTempo)/(maxTempo-minTempo)*(c.width-60);"
"let y=c.height-20-((dados[i]-minPress)/(maxPress-minPress))*(c.height-40);"
"if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);"
"}"
"ctx.stroke();"
"ctx.fillStyle='#666';"
"ctx.font='12px Arial';"
"ctx.fillText('Tempo (s)',c.width/2-30,c.height-5);"
"}"
"setInterval(atualizar,500);"
"atualizar();"
"</script>"
"</body></html>";

// Estrutura para manter o estado da conexão HTTP.
struct http_state {
    char *response;
    size_t len;
    size_t sent;
};

// Callback chamada quando dados HTTP foram enviados. Gerencia o envio de grandes respostas em pedaços.
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    
    if (hs->sent >= hs->len) {
        // Todos os dados foram enviados, fecha a conexão e libera a memória.
        tcp_close(tpcb);
        free(hs->response);
        free(hs);
        return ERR_OK;
    }
    
    // Envia o próximo pedaço da resposta, se houver.
    u16_t remaining = hs->len - hs->sent;
    u16_t to_send = remaining > tcp_sndbuf(tpcb) ? tcp_sndbuf(tpcb) : remaining;
    
    if (to_send > 0) {
        err_t err = tcp_write(tpcb, hs->response + hs->sent, to_send, TCP_WRITE_FLAG_COPY);
        if (err == ERR_OK) {
            tcp_output(tpcb);
        }
    }
    
    return ERR_OK;
}

// Função de callback para receber dados HTTP e rotear as requisições.
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        // Conexão fechada pelo cliente.
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        // Falha ao alocar memória para o estado HTTP.
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    
    hs->response = malloc(8192); // Aloca buffer para a resposta HTTP.
    if (!hs->response) {
        // Falha ao alocar memória para a resposta.
        free(hs);
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    
    printf("Requisicao recebida: %.50s\n", req);
    
    // Roteamento das requisições HTTP e preparação das respostas.
    if (strstr(req, "GET /dados")) {
        // Responde com dados dos sensores em formato JSON, aplicando offsets de calibração.
        char json_payload[256];
        uint32_t timestamp = to_ms_since_boot(get_absolute_time());
        
        float temp_com_offset = data.temperature + temp_offset;
        float umid_com_offset = data.humidity + umid_offset;
        float press_com_offset = global_pressure + (press_offset * 100); // Converter hPa para Pa
        
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{\"temperatura\":%.2f,\"umidade\":%.2f,\"pressao\":%.2f,\"timestamp\":%lu}",
            temp_com_offset, umid_com_offset, press_com_offset, timestamp);
            
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if (strstr(req, "GET /temp")) {
        // Envia a página HTML de temperatura.
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            (int)strlen(HTML_TEMP), HTML_TEMP);
    }
    else if (strstr(req, "GET /umid")) {
        // Envia a página HTML de umidade.
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            (int)strlen(HTML_UMID), HTML_UMID);
    }
    else if (strstr(req, "GET /press")) {
        // Envia a página HTML de pressão.
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            (int)strlen(HTML_PRESS), HTML_PRESS);
    }
    else if (strstr(req, "GET /offset")) {
    // Envia a página HTML para calibragem de offset.
    hs->len = snprintf(hs->response, 8192,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        (int)strlen(HTML_OFFSET), HTML_OFFSET);
    }
    else if (strstr(req, "POST /setoffset")) {  
        // Processa o POST para salvar os valores de offset.
        char *body = strstr(req, "\r\n\r\n");
        if (body) {
            body += 4;
            printf("Body recebido: %.100s\n", body);

            char *p;
            p = strstr(body, "temp_offset=");
            if (p) temp_offset = atof(p + strlen("temp_offset="));
            p = strstr(body, "umid_offset=");
            if (p) umid_offset = atof(p + strlen("umid_offset="));
            p = strstr(body, "press_offset=");
            if (p) press_offset = atof(p + strlen("press_offset="));

            printf("Novos offsets: temp=%.2f, umid=%.2f, press=%.2f\n", 
                temp_offset, umid_offset, press_offset);
        }
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: 25\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{\"status\":\"success\"}");
    }

    else if (strstr(req, "GET /getoffset")) {
        // Responde com os valores atuais de offset em JSON.
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{\"temp_offset\":%.2f,\"umid_offset\":%.2f,\"press_offset\":%.2f}",
            temp_offset, umid_offset, press_offset);
            
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if (strstr(req, "POST /config")) {
        // Processa o POST para salvar os limites de monitoramento.
        char *body = strstr(req, "\r\n\r\n");
        if (body) {
            body += 4;
            sscanf(body, "temp_min=%f&temp_max=%f&umid_min=%f&umid_max=%f&press_min=%f&press_max=%f",
                   &temp_min, &temp_max, &umid_min, &umid_max, &press_min, &press_max);
        }
        
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: 25\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{\"status\":\"success\"}");
    }
    else if (strstr(req, "GET /config")) {
        // Responde com os limites atuais em JSON.
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{\"temp_min\":%.1f,\"temp_max\":%.1f,\"umid_min\":%.1f,\"umid_max\":%.1f,\"press_min\":%.1f,\"press_max\":%.1f}",
            temp_min, temp_max, umid_min, umid_max, press_min, press_max);
            
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if (strstr(req, "GET /pagina")) {
        // Responde com o número da página atual em JSON.
        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: 20\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{\"pagina\":%d}", pagina_atual);
    }
    else {
        // Responde com a página HTML padrão ou a página específica com base em `pagina_atual`.
        const char* pagina;
        if (pagina_atual == 0) pagina = HTML_BODY;
        else if (pagina_atual == 1) pagina = HTML_TEMP;
        else if (pagina_atual == 2) pagina = HTML_UMID;
        else if (pagina_atual == 3) pagina = HTML_PRESS;
        else if (pagina_atual == 4) pagina = HTML_OFFSET;
        else pagina = HTML_BODY;

        hs->len = snprintf(hs->response, 8192,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            (int)strlen(pagina), pagina);
    }
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    hs->sent = 0;
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    
    // Envia o primeiro pedaço da resposta.
    u16_t to_send = hs->len > tcp_sndbuf(tpcb) ? tcp_sndbuf(tpcb) : hs->len;
    err_t write_err = tcp_write(tpcb, hs->response, to_send, TCP_WRITE_FLAG_COPY);
    if (write_err == ERR_OK) {
        tcp_output(tpcb);
    }
    
    return ERR_OK;
}

// Callback para novas conexões TCP, configura a função de recebimento de dados.
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    tcp_err(newpcb, NULL);
    return ERR_OK;
}

// Inicia o servidor HTTP na porta 80.
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    
    err_t err = tcp_bind(pcb, IP_ADDR_ANY, 80);
    if (err != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80: %d\n", err);
        tcp_close(pcb);
        return;
    }
    
    pcb = tcp_listen(pcb);
    if (!pcb) {
        printf("Erro ao iniciar listen\n");
        return;
    }
    
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP iniciado na porta 80\n");
}

// Controla o comportamento do buzzer, fazendo-o bipar por um período.
void atualiza_buzzer() {
    if (!buzzer_ativo) return;

    if (absolute_time_diff_us(ultimo_bip, get_absolute_time()) >= 250000) { // Alterna a cada 250ms
        ultimo_bip = get_absolute_time();
        estado_buzzer = !estado_buzzer;
        pwm_set_gpio_level(buzzer1, estado_buzzer ? 250 : 0);
        pwm_set_gpio_level(buzzer2, estado_buzzer ? 250 : 0);

        ciclos_buzzer++;
        if (ciclos_buzzer >= 8) {  // 4 ciclos ON/OFF = 1 segundo de bips.
            buzzer_ativo = false;
            pwm_set_gpio_level(buzzer1, 0);
            pwm_set_gpio_level(buzzer2, 0);
            ciclos_buzzer = 0;
        }
    }
}

// Realiza a análise dos dados dos sensores, ativa alarmes (LEDs e buzzer) se os limites forem excedidos.
void analise() {
    float temp_com_offset = data.temperature + temp_offset;
    float umid_com_offset = data.humidity + umid_offset;
    float press_com_offset = (global_pressure/100) + press_offset;
    printf("DEBUG - Valores originais: temp=%.2f, umid=%.2f, press=%.2f\n",
           data.temperature, data.humidity, global_pressure/100.0f);
    printf("DEBUG - Valores com offset: temp=%.2f, umid=%.2f, press=%.2f\n",
           temp_com_offset, umid_com_offset, press_com_offset);
    if(temp_com_offset > temp_max) {
        printf("Temperatura elevada\n");
        num1(255,0,0); // LED WS2812 exibe "1" em vermelho.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(VERMELHO, true); // LED Vermelho aceso.
        gpio_put(AZUL, false);
        gpio_put(VERDE, false);
    }
    else if(temp_com_offset < temp_min) {
        printf("Sistema operando em temperaturas críticas\n");
        num0(0,0,255); // LED WS2812 exibe "0" em azul.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(AZUL, true); // LED Azul aceso.
        gpio_put(VERMELHO, false);
        gpio_put(VERDE, false);
    }
    else if(umid_com_offset > umid_max) {
        printf("Umidade muito elevada\n");
        num3(0,255,255); // LED WS2812 exibe "3" em ciano.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(AZUL, true); // LED Azul e Verde (ciano) aceso.
        gpio_put(VERMELHO, false);
        gpio_put(VERDE, true);
    }
    else if(umid_com_offset < umid_min) {
        printf("Umidade em situação crítica\n");
        num2(255,165,0); // LED WS2812 exibe "2" em laranja.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(VERMELHO, true); // LED Vermelho e Verde (amarelo) aceso.
        gpio_put(AZUL, false);
        gpio_put(VERDE, true);
    }
    else if(press_com_offset > press_max) {
        printf("Pressão muito alta\n");
        num5(0,100,255); // LED WS2812 exibe "5" em azul escuro.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(VERMELHO, true); // LED Vermelho e Azul (magenta) aceso.
        gpio_put(AZUL, true);
        gpio_put(VERDE, false);
    }
    else if(press_com_offset < press_min) {
        printf("Pressão muito baixa\n");
        num6(255,200,0); // LED WS2812 exibe "6" em amarelo.
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
        gpio_put(AZUL, true); // LED Azul aceso.
        gpio_put(VERMELHO, false);
        gpio_put(VERDE, false);
    }
    else {
        num4(0,255,0); // LED WS2812 exibe "4" em verde (estado normal).
        if (buzzer_ativo) {
            buzzer_ativo = false;
            pwm_set_gpio_level(buzzer1, 0);
            pwm_set_gpio_level(buzzer2, 0);
        }
        gpio_put(VERDE, true); // LED Verde aceso.
        gpio_put(AZUL, false);
        gpio_put(VERMELHO, false);
    }
}

// Função principal do programa.
int main() {

    inicia_pinos(); // Inicializa todos os pinos e periféricos.

    // Inicializa os sensores BMP280 e AHT20.
    bmp280_init(I2C_PORT);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT, &params);

    aht20_reset(I2C_PORT_DISP);
    aht20_init(I2C_PORT_DISP);

    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    // Inicializa e conecta ao Wi-Fi.
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(1000);
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha ao conectar ao Wi-Fi, tentando novamente...\n");
        sleep_ms(2000);
    }
    printf("Conectado ao Wi-Fi!\n");

    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    start_http_server(); // Inicia o servidor HTTP.

    // Loop principal do programa.
    while (1) {
        cyw43_arch_poll(); // Permite que a pilha de rede Wi-Fi processe eventos.
        atualiza_buzzer(); // Gerencia o estado do buzzer.
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure); // Lê dados brutos do BMP280.
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params); // Converte temperatura.
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params); // Converte pressão.
        global_pressure = pressure; // Armazena a pressão global.
        
        double altitude = calculate_altitude(pressure); // Calcula a altitude.
        
        printf("Altitude estimada: %.2f m\n", altitude);
        float press_com_offset = (global_pressure/100) + press_offset;
        printf("Pressao = %.3f kPa (Corrigido: %.2f)\n", (pressure / 1000.0), press_com_offset);
        // Lê dados do AHT20 e realiza a análise.
        if (aht20_read(I2C_PORT_DISP, &data))
        {
            float temp_com_offset = data.temperature + temp_offset;
            float umid_com_offset = data.humidity + umid_offset;
            
            analise(); // Realiza a análise dos valores e atualiza os indicadores (LEDs, buzzer).
            printf("Temperatura AHT: %.2f (Corrigido: %.2f) C\n", data.temperature, temp_com_offset);
            printf("Umidade: %.2f (Corrigido: %.2f) %%\n\n\n", data.humidity, umid_com_offset);
        }
        else
        {
            printf("Erro na leitura do AHT10!\n\n\n");
        }
        sleep_ms(500); // Pequena pausa antes da próxima leitura.
    }
    cyw43_arch_deinit(); // Desinicializa o Wi-Fi.
    return 0;
}
