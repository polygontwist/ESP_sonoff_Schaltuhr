//---------------------------------WWW------------------------------
//ESP8266

//HTML \r\n \t
const String indexHTM="<!DOCTYPE html><html><head>\r\n"
"<meta charset=\"utf-8\"/>\r\n"
"<link rel=\"shortcut icon\" href=\"favicon.ico\">\r\n"
"<link rel=\"STYLESHEET\" type=\"text/css\" href=\"style.css\">\r\n"
"<script type=\"text/javascript\" src=\"system.js\"></script>\r\n"
"</head><body>\r\n"
"<h1>$h1gtag</h1>\r\n"
"<div id=\"actions\"></div>\r\n"
"<div id=\"timersetting\"></div>\r\n"
"<div id=\"sysinfo\"></div>\r\n"
"<div id=\"filelist\">\r\n"
"$filelist"
"</div>\r\n"
"<form class=\"upload\" method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
"<input type=\"file\" name=\"upload\" required>\r\n"
"<input type=\"submit\" value=\"Upload\" class=\"button\">\n</form>\r\n"
"</body></html>\r\n"
;

/*
const String indexHTM="<!DOCTYPE html><html><head>\r\n"
"<meta charset=\"utf-8\"/>\r\n"
"<link rel=\"shortcut icon\" href=\"favicon.ico\">\r\n"
"<link rel=\"STYLESHEET\" type=\"text/css\" href=\"style.css\">\r\n"
"<script type=\"text/javascript\" src=\"system.js\"></script>\r\n"
"</head><body>\r\n"
"<h1>$h1gtag</h1>\r\n"
"<p class=\"buttons\">\r\n"
"<a href=\"#\" data-OF='{\"action\":\"ON\"}'>ON</a>"
"<a href=\"#\" data-OF='{\"action\":\"OFF\"}'>OFF</a>"
"<a href=\"#\" data-OF='{\"action\":\"LEDON\"}'>Led:on</a>"
"<a href=\"#\" data-OF='{\"action\":\"LEDOFF\"}'>Led:off</a>"
"</p>\r\n"
"<div id=\"timersetting\"></div>\r\n"
"<div id=\"sysinfo\"></div>\r\n"
"<div id=\"filelist\">\r\n"
"$filelist"
"</div>\r\n"
"<form class=\"upload\" method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
"<input type=\"file\" name=\"upload\" required>\r\n"
"<input type=\"submit\" value=\"Upload\" class=\"button\">\n</form>\r\n"
"</body></html>\r\n"
;

*/
