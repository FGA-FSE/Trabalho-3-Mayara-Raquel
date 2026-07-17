/*
 * task_http_server - serve os dados por HTTP (SoftAP em 192.168.4.1).
 *   GET  /            -> pagina HTML (leitura + legenda + ajuste de faixas)
 *   GET  /api/status  -> JSON com temperatura/umidade/pressao/status
 *   GET  /api/config  -> JSON com as faixas atuais (ideal_min/ideal_max/banda)
 *   POST /api/config  -> altera as faixas (form-encoded), aplica na hora
 * O esp_http_server roda em sua propria task interna.
 */
#include "tasks.h"
#include "shared_state.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "http";

/* Pagina HTML. Convencoes p/ nao precisar escapar no C:
 *  - atributos HTML sempre com aspas simples;
 *  - strings JS sempre com aspas simples ou template literals (backticks). */
static const char PAGE[] =
"<!doctype html><html lang='pt-br'><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Incubadora</title><style>"
"body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:1.5rem}"
".card{max-width:440px;margin:1rem auto;background:#1c1c1c;border-radius:16px;padding:1.4rem}"
".temp{font-size:3.4rem;font-weight:700;margin:.3rem}"
".badge{display:inline-block;padding:.3rem 1.2rem;border-radius:999px;font-weight:700;margin:.6rem;color:#111}"
".row{display:flex;justify-content:space-around;margin-top:1.2rem}"
".row span{display:block;font-size:1.4rem;font-weight:700}"
"h3{margin:.2rem 0 .8rem;font-size:1rem;color:#bbb;text-align:left}"
".lg{display:flex;align-items:center;gap:.5rem;text-align:left;margin:.35rem 0;font-size:.9rem}"
".lg i{width:14px;height:14px;border-radius:50%;flex:0 0 auto}"
".lg b{min-width:110px}"
".lg em{color:#9aa;font-style:normal}"
"form{display:flex;flex-wrap:wrap;gap:.5rem;align-items:flex-end;justify-content:center}"
"label{display:flex;flex-direction:column;font-size:.75rem;color:#bbb;text-align:left}"
"input{width:74px;padding:.4rem;margin-top:.2rem;border-radius:8px;border:1px solid #444;"
"background:#222;color:#eee;font-size:1rem}"
"button{padding:.55rem 1rem;border:0;border-radius:8px;background:#2ecc71;color:#062;"
"font-weight:700;cursor:pointer}"
"#msg{display:block;margin-top:.6rem;font-size:.85rem;min-height:1rem}"
"small{color:#888}</style></head><body>"

"<div class='card'>"
"<h2>Incubadora &mdash; Temperatura</h2>"
"<div class='temp' id='t'>--</div>"
"<div class='badge' id='b'>--</div>"
"<div class='row'><div><span id='h'>--</span>Umidade</div>"
"<div><span id='p'>--</span>Pressao</div>"
"<div><span id='ht'>--</span>Aquecedor</div></div>"
"<small id='u'>conectando...</small></div>"

"<div class='card'><h3>Legenda</h3><div id='leg'></div></div>"

"<div class='card'><h3>Ajustar faixas (graus C)</h3>"
"<form id='cfg'>"
"<label>Ideal min<input name='ideal_min' type='number' step='0.1' required></label>"
"<label>Ideal max<input name='ideal_max' type='number' step='0.1' required></label>"
"<label>Banda alerta<input name='banda' type='number' step='0.1' min='0' required></label>"
"<button type='submit'>Salvar</button>"
"</form><span id='msg'></span></div>"

"<script>"
"const C={ok:'#2ecc71',warning:'#f1c40f',alarm:'#e74c3c',error:'#3498db',init:'#3498db'};"
"const N={ok:'IDEAL',warning:'ALERTA',alarm:'FORA DA FAIXA',error:'ERRO SENSOR',init:'INICIANDO'};"
"const f1=x=>Number(x).toFixed(1);"
"function legend(c){"
"const lo=c.ideal_min,hi=c.ideal_max,b=c.banda;"
"const rows=["
"['#2ecc71','IDEAL',f1(lo)+' a '+f1(hi)],"
"['#f1c40f','ALERTA',f1(lo-b)+' a '+f1(lo)+'  e  '+f1(hi)+' a '+f1(hi+b)],"
"['#e74c3c','FORA DA FAIXA','< '+f1(lo-b)+'  ou  > '+f1(hi+b)],"
"['#3498db','ERRO (piscando)','falha na leitura do sensor'],"
"['#3498db','INICIANDO (fixo)','no boot, antes da 1a leitura']];"
"return rows.map(r=>`<div class='lg'><i style='background:${r[0]}'></i><b>${r[1]}</b><em>${r[2]}</em></div>`).join('');"
"}"
"function fillCfg(c){const f=document.getElementById('cfg');"
"f.ideal_min.value=c.ideal_min;f.ideal_max.value=c.ideal_max;f.banda.value=c.banda;"
"document.getElementById('leg').innerHTML=legend(c);}"
"async function loadCfg(){try{const r=await fetch('/api/config');fillCfg(await r.json());}catch(e){}}"
"async function u(){try{const r=await fetch('/api/status');const d=await r.json();"
"document.getElementById('t').textContent=d.temperature.toFixed(2)+' C';"
"document.getElementById('h').textContent=(d.humidity===null?'N/A':d.humidity.toFixed(0)+'%');"
"document.getElementById('p').textContent=d.pressure.toFixed(0)+' hPa';"
"const ht=document.getElementById('ht');ht.textContent=d.heater?'LIGADO':'DESLIGADO';"
"ht.style.fontSize='1rem';ht.style.color=d.heater?'#2ecc71':'#888';"
"const b=document.getElementById('b');b.textContent=N[d.status]||d.status;"
"b.style.background=C[d.status]||'#888';"
"document.getElementById('u').textContent='atualizado agora';"
"}catch(e){document.getElementById('u').textContent='sem conexao';}}"
"document.getElementById('cfg').addEventListener('submit',async e=>{"
"e.preventDefault();const f=e.target;const m=document.getElementById('msg');"
"const body='ideal_min='+f.ideal_min.value+'&ideal_max='+f.ideal_max.value+'&banda='+f.banda.value;"
"try{const r=await fetch('/api/config',{method:'POST',"
"headers:{'Content-Type':'application/x-www-form-urlencoded'},body});"
"if(r.ok){fillCfg(await r.json());m.textContent='Faixas salvas!';m.style.color='#2ecc71';}"
"else{m.textContent='Faixa invalida (min < max e banda >= 0).';m.style.color='#e74c3c';}"
"}catch(e){m.textContent='sem conexao';m.style.color='#e74c3c';}});"
"loadCfg();u();setInterval(u,2000);"
"</script></body></html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, PAGE);
    return ESP_OK;
}

static esp_err_t api_handler(httpd_req_t *req)
{
    sensor_data_t d;
    shared_state_get(&d);
    incub_status_t st = compute_status(&d);

    char buf[256];
    if (d.has_humidity)
        snprintf(buf, sizeof(buf),
                 "{\"temperature\":%.2f,\"humidity\":%.1f,\"pressure\":%.1f,"
                 "\"heater\":%s,\"status\":\"%s\",\"sensor_ok\":%s}",
                 d.temperature, d.humidity, d.pressure,
                 d.heater_on ? "true" : "false", status_str(st),
                 d.sensor_ok ? "true" : "false");
    else
        snprintf(buf, sizeof(buf),
                 "{\"temperature\":%.2f,\"humidity\":null,\"pressure\":%.1f,"
                 "\"heater\":%s,\"status\":\"%s\",\"sensor_ok\":%s}",
                 d.temperature, d.pressure,
                 d.heater_on ? "true" : "false", status_str(st),
                 d.sensor_ok ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

/* Serializa as faixas atuais (usado pelo GET e como resposta do POST). */
static esp_err_t send_config(httpd_req_t *req)
{
    temp_limits_t lim;
    shared_state_get_limits(&lim);

    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"ideal_min\":%.2f,\"ideal_max\":%.2f,\"banda\":%.2f}",
             lim.ideal_min, lim.ideal_max, lim.alerta_banda);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    return send_config(req);
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    char body[160];
    int total = req->content_len;
    if (total <= 0 || total >= (int)sizeof(body)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "corpo invalido");
        return ESP_FAIL;
    }
    int r = httpd_req_recv(req, body, total);
    if (r <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "falha ao ler corpo");
        return ESP_FAIL;
    }
    body[r] = '\0';

    /* parte dos valores atuais e sobrescreve os campos recebidos */
    temp_limits_t lim;
    shared_state_get_limits(&lim);
    char v[24];
    if (httpd_query_key_value(body, "ideal_min", v, sizeof(v)) == ESP_OK) lim.ideal_min    = atof(v);
    if (httpd_query_key_value(body, "ideal_max", v, sizeof(v)) == ESP_OK) lim.ideal_max    = atof(v);
    if (httpd_query_key_value(body, "banda",     v, sizeof(v)) == ESP_OK) lim.alerta_banda = atof(v);

    if (!shared_state_set_limits(&lim)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "faixa invalida (exige ideal_min < ideal_max e banda >= 0)");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "faixas atualizadas via web: ideal %.2f..%.2f, banda %.2f",
             lim.ideal_min, lim.ideal_max, lim.alerta_banda);
    return send_config(req);   /* devolve os novos valores */
}

void http_server_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t srv = NULL;

    if (httpd_start(&srv, &cfg) == ESP_OK) {
        httpd_uri_t u_root = { .uri = "/",           .method = HTTP_GET,  .handler = root_handler        };
        httpd_uri_t u_api  = { .uri = "/api/status", .method = HTTP_GET,  .handler = api_handler         };
        httpd_uri_t u_cfg_g= { .uri = "/api/config", .method = HTTP_GET,  .handler = config_get_handler  };
        httpd_uri_t u_cfg_p= { .uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler };
        httpd_register_uri_handler(srv, &u_root);
        httpd_register_uri_handler(srv, &u_api);
        httpd_register_uri_handler(srv, &u_cfg_g);
        httpd_register_uri_handler(srv, &u_cfg_p);
        ESP_LOGI(TAG, "HTTP server ativo: http://192.168.4.1/  (JSON em /api/status, faixas em /api/config)");
    } else {
        ESP_LOGE(TAG, "falha ao iniciar o HTTP server");
    }
}
