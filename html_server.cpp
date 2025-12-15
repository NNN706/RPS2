#include "html_server.h"
#include <fstream>
#include <sstream>

bool createHtmlSite(const std::vector<int>& currentArray) {

    // ---------- CSS ----------
    std::ofstream css("style.css");
    if (!css.is_open()) return false;

    css <<
        "body{font-family:Arial;background:#fef6f0;padding:20px}\n"
        "header{background:#cde4e3;padding:12px;border-radius:10px;text-align:center}\n"
        "button{margin:6px;padding:10px 16px;border-radius:8px;border:none;background:#f4a261;color:#fff;cursor:pointer}\n"
        "button:hover{background:#e76f51}\n"
        ".array{background:#fff7f0;padding:10px;border-radius:8px;margin-top:10px}\n"
        "input{padding:8px;border-radius:6px;border:1px solid #ccc;width:300px}\n"
        "#toast{position:fixed;bottom:20px;right:20px;background:#a8dadc;padding:10px;border-radius:8px;opacity:0;transition:.3s}\n"
        "#toast.show{opacity:1}\n";

    css.close();

    // ---------- ARRAY ----------
    std::ostringstream arr;
    for (size_t i = 0; i < currentArray.size(); ++i) {
        if (i) arr << ",";
        arr << currentArray[i];
    }

    // ---------- HTML ----------
    std::string html =
        u8"<!DOCTYPE html><html lang='ru'><head>"
        u8"<meta charset='UTF-8'>"
        u8"<title>Блочная сортировка</title>"
        u8"<link rel='stylesheet' href='style.css'></head><body>"

        u8"<header><h1>Система блочной сортировки</h1></header>"

        u8"<input id='manual' placeholder='Введите числа через запятую'>"
        u8"<button onclick='applyManual()'>Применить</button>"
        u8"<button onclick='autoFill()'>Авто</button>"
        u8"<button onclick='saveSQL()'>Сохранить в SQL</button>"

        u8"<div class='array' id='current'>Текущий массив: " + arr.str() + u8"</div>"
        u8"<div class='array' id='sorted'>Отсортированный массив: " + arr.str() + u8"</div>"

        u8"<div id='toast'></div>"

        u8"<script>"
        u8"function toast(t){let e=document.getElementById('toast');e.innerText=t;e.classList.add('show');setTimeout(()=>e.classList.remove('show'),2000)}"
        u8"function sort(a){a=a.slice().sort((x,y)=>x-y);document.getElementById('sorted').innerText='Отсортированный массив: '+a.join(',');return a}"
        u8"function applyManual(){let v=document.getElementById('manual').value;if(!v)return;"
        u8"document.getElementById('current').innerText='Текущий массив: '+v;"
        u8"sort(v.split(',').map(Number));toast('Обновлено')}"
        u8"function autoFill(){let a=[];for(let i=0;i<10;i++)a.push(Math.floor(Math.random()*100));"
        u8"document.getElementById('current').innerText='Текущий массив: '+a.join(',');sort(a);toast('Авто')}"
        u8"function saveSQL(){let t=document.getElementById('sorted').innerText.split(': ')[1];"
        u8"fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({array:t})})"
        u8".then(()=>toast('Сохранено в SQL')).catch(()=>toast('Ошибка SQL'))}"
        u8"</script></body></html>";

    std::ofstream file("index.html", std::ios::binary);
    file.write(html.c_str(), html.size());
    file.close();

    return true;
}
