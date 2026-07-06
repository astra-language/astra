/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "astra_sdk.h"
#include "vm.h"
#include "common.h"
#include "error.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_set>

#ifdef _WIN32
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

std::string globalCss = "";
std::string globalJs = "";
std::string currentFileName = "index.html";
std::string globalTitle = "";
std::string globalFonts = "";

Value createStringValue(const std::string& s) {
    Value v; v.type = VAL_STR; v.isInitialized = true; v.str = s;
    return v;
}


static bool expectStr(AstraVM* vm, const Value& v, const char* funcName, const char* argName) {
    if (v.type != VAL_STR) {
        if (g_reportError) g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine,
            std::string(funcName) + "() expects a string for '" + argName + "'");
        return false;
    }
    return true;
}

void web_set_filename(AstraVM* vm) {
    Value name = vm->pop();
    if (!expectStr(vm, name, "set_filename", "name")) return;
    if (name.str.empty()) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            "set_filename() cannot be empty");
        return;
    }
    currentFileName = name.str;
}

void web_style(AstraVM* vm) {
    Value css = vm->pop();
    if (!expectStr(vm, css, "style", "css")) return;
    globalCss += "\t\t" + css.str + "\n";
}

void web_js(AstraVM* vm) {
    Value script = vm->pop();
    if (!expectStr(vm, script, "js", "script")) return;
    globalJs += script.str + "\n";
}

void web_div(AstraVM* vm) {
    Value content = vm->pop();
    Value className = vm->pop();
    if (!expectStr(vm, content, "div", "content") || !expectStr(vm, className, "div", "class")) {
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }

    std::string result = "<div class='" + className.str + "'>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) {
        result += "\t" + line + "\n";
    }
    result += "</div>\n";

    vm->push(createStringValue(result));
}

void web_h1(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "h1", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<h1>" + text.str + "</h1>\n"));
}

void web_para(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "para", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<p>" + text.str + "</p>\n"));
}

void web_btn(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "btn", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<button class='btn'>" + text.str + "</button>\n"));
}

void web_img(AstraVM* vm) {
    Value alt = vm->pop();
    Value src = vm->pop();
    if (!expectStr(vm, alt, "img", "alt") || !expectStr(vm, src, "img", "src")) {
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    if (src.str.empty()) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "img() src cannot be empty");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }
    vm->push(createStringValue("<img src='" + src.str + "' alt='" + alt.str +
                                "' style='width:100%; border-radius:10px;'>\n"));
}

void web_link(AstraVM* vm) {
    Value href = vm->pop();
    Value text = vm->pop();
    if (!expectStr(vm, href, "link", "href") || !expectStr(vm, text, "link", "text")) {
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    vm->push(createStringValue("<a href='" + href.str + "'>" + text.str + "</a>\n"));
}

void web_hover(AstraVM* vm) {
    Value style = vm->pop();
    Value selector = vm->pop();
    if (!expectStr(vm, style, "hover", "style") || !expectStr(vm, selector, "hover", "selector")) return;
    globalCss += "\t\t" + selector.str + ":hover { " + style.str + " }\n";
}

void web_input(AstraVM* vm) {
    Value placeholder = vm->pop();
    Value type = vm->pop();
    if (!expectStr(vm, placeholder, "input", "placeholder") || !expectStr(vm, type, "input", "type")) {
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    std::string html = "<input type='" + type.str + "' placeholder='" + placeholder.str +
                       "' style='display:block; width:100%; margin:15px 0; padding:12px; border:1px solid #ccc; border-radius:8px;'>\n";
    vm->push(createStringValue(html));
}

void web_save(AstraVM* vm) {
    Value finalHtml = vm->pop();
    if (!expectStr(vm, finalHtml, "save", "html")) return;

    std::ofstream f(currentFileName);
    if (!f.is_open()) {
        if (g_reportError) g_reportError(ErrCode::NET_FILE_ERROR, vm->currentLine,
            "save() could not open '" + currentFileName + "' for writing");
        return;
    }

    // ── Deduplicate CSS ──────────────────────────────
    std::string dedupCss;
    std::unordered_set<std::string> seenCss;
    std::istringstream cssStream(globalCss);
    std::string cssLine;
    while (std::getline(cssStream, cssLine)) {
        std::string trimmed = cssLine;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        if (!trimmed.empty() && seenCss.find(trimmed) == seenCss.end()) {
            seenCss.insert(trimmed);
            dedupCss += cssLine + "\n";
        }
    }

    // ── Deduplicate JS ───────────────────────────────
    std::string dedupJs;
    std::unordered_set<std::string> seenJs;
    std::istringstream jsStream(globalJs);
    std::string jsLine;
    while (std::getline(jsStream, jsLine)) {
        std::string trimmed = jsLine;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        if (!trimmed.empty() && seenJs.find(trimmed) == seenJs.end()) {
            seenJs.insert(trimmed);
            dedupJs += jsLine + "\n";
        }
    }

    f << "<!DOCTYPE html>\n<html>\n<head>\n";
    if (!globalTitle.empty()) {
        f << "\t<title>" << globalTitle << "</title>\n";
        f << "\t<meta charset='UTF-8'>\n";
        f << "\t<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    }
    if (!globalFonts.empty()) {
        f << globalFonts;
    }
    f << "\t<style>\n" << dedupCss << "\t</style>\n</head>\n<body>\n";

    std::string html = finalHtml.str;
    size_t start = 0;
    size_t end = html.find('\n');
    while (end != std::string::npos) {
        std::string line = html.substr(start, end - start);
        if (!line.empty()) { f << "\t\t" << line << "\n"; }
        start = end + 1;
        end = html.find('\n', start);
    }

    f << "</body>\n<script>\n" << dedupJs << "</script>\n</html>";

    if (f.fail()) {
        if (g_reportError) g_reportError(ErrCode::NET_FILE_ERROR, vm->currentLine,
            "save() encountered a write error on '" + currentFileName + "'");
    }
    f.close();

    globalCss = "";
    globalJs = "";
    currentFileName = "index.html";
}

void web_meta(AstraVM* vm) {
    Value title = vm->pop();
    if (!expectStr(vm, title, "meta", "title")) return;
    globalTitle = title.str;
}

void web_font(AstraVM* vm) {
    Value name = vm->pop();
    if (!expectStr(vm, name, "font", "name")) return;
    std::string fontName = name.str;
    std::replace(fontName.begin(), fontName.end(), ' ', '+');
    globalFonts += "\t<link href='https://fonts.googleapis.com/css2?family="
               + fontName + ":wght@400;700&display=swap' rel='stylesheet'>\n";
}

void web_h2(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "h2", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<h2>" + text.str + "</h2>\n"));
}

void web_h3(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "h3", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<h3>" + text.str + "</h3>\n"));
}

void web_nav(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "nav", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<nav>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</nav>\n";
    vm->push(createStringValue(result));
}

void web_ul(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "ul", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<ul>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</ul>\n";
    vm->push(createStringValue(result));
}

void web_li(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "li", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<li>" + text.str + "</li>\n"));
}

void web_section(AstraVM* vm) {
    Value content = vm->pop();
    Value className = vm->pop();
    if (!expectStr(vm, content, "section", "content") || !expectStr(vm, className, "section", "class")) {
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    std::string result = "<section class='" + className.str + "'>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</section>\n";
    vm->push(createStringValue(result));
}

void web_header(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "header", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<header>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</header>\n";
    vm->push(createStringValue(result));
}

void web_footer(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "footer", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<footer>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</footer>\n";
    vm->push(createStringValue(result));
}

void web_table(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "table", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<table>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</table>\n";
    vm->push(createStringValue(result));
}

void web_tr(AstraVM* vm) {
    Value content = vm->pop();
    if (!expectStr(vm, content, "tr", "content")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    std::string result = "<tr>\n";
    std::stringstream ss(content.str);
    std::string line;
    while (std::getline(ss, line)) result += "\t" + line + "\n";
    result += "</tr>\n";
    vm->push(createStringValue(result));
}

void web_td(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "td", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<td>" + text.str + "</td>\n"));
}

void web_th(AstraVM* vm) {
    Value text = vm->pop();
    if (!expectStr(vm, text, "th", "text")) { { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); } return; }
    vm->push(createStringValue("<th>" + text.str + "</th>\n"));
}

void web_css_link(AstraVM* vm) {
    Value url = vm->pop();
    if (!expectStr(vm, url, "clink", "url")) return;
    globalFonts += "\t<link rel='stylesheet' href='" + url.str + "'>\n";
}

void web_script_link(AstraVM* vm) {
    Value url = vm->pop();
    if (!expectStr(vm, url, "slink", "url")) return;
    globalFonts += "\t<script src='" + url.str + "'></script>\n";
}

ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("style", web_style);
    reg("h1", web_h1);
    reg("div", web_div);
    reg("save", web_save);
    reg("set_filename", web_set_filename);
    reg("btn", web_btn);
    reg("para", web_para);
    reg("img", web_img);
    reg("hover", web_hover);
    reg("js", web_js);
    reg("input", web_input);
    reg("link", web_link);
    reg("meta", web_meta);
    reg("font", web_font);
    reg("h2", web_h2);
    reg("h3", web_h3);
    reg("nav", web_nav);
    reg("ul", web_ul);
    reg("li", web_li);
    reg("section", web_section);
    reg("header",  web_header);
    reg("footer",  web_footer);
    reg("table", web_table);
    reg("tr",    web_tr);
    reg("td",    web_td);
    reg("th",    web_th);
    reg("clink",    web_css_link);
    reg("slink", web_script_link);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "h1(text)|Heading tag <h1>\n"
            "h2(text)|Heading tag <h2>\n"
            "h3(text)|Heading tag <h3>\n"
            "para(text)|Paragraph tag <p>\n"
            "btn(text)|Button element\n"
            "div(class,content)|Div with class\n"
            "img(alt,src)|Image element\n"
            "link(text,href)|Anchor tag <a>\n"
            "input(type,placeholder)|Input field\n"
            "style(css)|Add CSS rule\n"
            "hover(selector,style)|Hover CSS rule\n"
            "js(script)|Add JavaScript\n"
            "set_filename(name)|Set output filename\n"
            "save(html)|Save HTML file\n"
            "meta(title)|Set page title + viewport\n"
            "font(name)|Load Google Font\n"
            "nav(content)|Navigation bar <nav>\n"
            "ul(content)|Unordered list <ul>\n"
            "li(text)|List item <li>\n"
            "section(class,content)|Section tag\n"
            "header(content)|Header tag\n"
            "footer(content)|Footer tag\n"
            "table(content)|Table element\n"
            "tr(content)|Table row\n"
            "td(text)|Table cell\n"
            "th(text)|Table header cell\n"
            "clink(url)|External CSS link\n"
            "slink(url)|External JS script\n";
    }
    return "Web_Module_Active";
}