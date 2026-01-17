const i18n = {
  en: { title: "Calculator", compute: "Compute", store: "Store", recall: "Recall", clear: "Clear", history: "History", preview: "Syntax Preview", shortcuts: "Shortcuts: Enter=Compute, Ctrl+M=Store, Ctrl+R=Recall, Ctrl+H=Toggle History" },
  es: { title: "Calculadora", compute: "Calcular", store: "Guardar", recall: "Recuperar", clear: "Limpiar", history: "Historial", preview: "Vista de Sintaxis", shortcuts: "Atajos: Enter=Calcular, Ctrl+M=Guardar, Ctrl+R=Recuperar, Ctrl+H=Historial" }
}
const expr = document.getElementById("expr")
const computeBtn = document.getElementById("compute")
const storeBtn = document.getElementById("store")
const recallBtn = document.getElementById("recall")
const clearBtn = document.getElementById("clear")
const output = document.getElementById("output")
const historyList = document.getElementById("history")
const suggestions = document.getElementById("suggestions")
const langSel = document.getElementById("lang")
const themeSel = document.getElementById("theme")
const codePreview = document.getElementById("code-preview")
let historyData = JSON.parse(localStorage.getItem("calc.history") || "[]")
let memStore = JSON.parse(localStorage.getItem("calc.mem") || "{}")
function setLang(l) { document.querySelectorAll("[data-i18n]").forEach(el => { el.textContent = i18n[l][el.getAttribute("data-i18n")] }) }
function setTheme(t) { document.body.className = t === "dark" ? "dark" : "" }
langSel.value = localStorage.getItem("calc.lang") || "en"
themeSel.value = localStorage.getItem("calc.theme") || "light"
setLang(langSel.value)
setTheme(themeSel.value)
langSel.addEventListener("change", () => { localStorage.setItem("calc.lang", langSel.value); setLang(langSel.value) })
themeSel.addEventListener("change", () => { localStorage.setItem("calc.theme", themeSel.value); setTheme(themeSel.value) })
function pushHistory(item) { historyData.unshift(item); if (historyData.length > 100) historyData.pop(); localStorage.setItem("calc.history", JSON.stringify(historyData)); renderHistory() }
function renderHistory() { historyList.innerHTML = ""; historyData.forEach(h => { const li = document.createElement("li"); li.textContent = h; historyList.appendChild(li) }) }
renderHistory()
function suggest(text) { const items = []; const base = ["sin(", "cos(", "tan(", "log(", "ln(", "sqrt(", "pow(,", "convert ,", "add ,", "subtract ,", "multiply ,", "divide "]; base.forEach(b => { if (b.startsWith(text)) items.push(b) }); return items.slice(0, 5) }
expr.addEventListener("input", () => { const s = suggest(expr.value.trim()); suggestions.innerHTML = ""; s.forEach(v => { const li = document.createElement("li"); li.textContent = v; suggestions.appendChild(li) }) })
suggestions.addEventListener("click", e => { if (e.target.tagName === "LI") { expr.value = e.target.textContent; suggestions.innerHTML = "" } })
function toNumber(x) { const v = parseFloat(x); return isNaN(v) ? 0 : v }
function evalMath(s) {
  try {
    const m = s.trim()
    if (/^convert\s+([\d\.\-]+)\s+([a-zA-Z]+)\s+to\s+([a-zA-Z]+)$/i.test(m)) { const [, n, f, t] = m.match(/^convert\s+([\d\.\-]+)\s+([a-zA-Z]+)\s+to\s+([a-zA-Z]+)$/i); return convert(toNumber(n), f, t) }
    if (/^add\s+(.+)\s+and\s+(.+)$/i.test(m)) { const [, a, b] = m.match(/^add\s+(.+)\s+and\s+(.+)$/i); return toNumber(a) + toNumber(b) }
    if (/^subtract\s+(.+)\s+from\s+(.+)$/i.test(m)) { const [, a, b] = m.match(/^subtract\s+(.+)\s+from\s+(.+)$/i); return toNumber(b) - toNumber(a) }
    if (/^multiply\s+(.+)\s+by\s+(.+)$/i.test(m)) { const [, a, b] = m.match(/^multiply\s+(.+)\s+by\s+(.+)$/i); return toNumber(a) * toNumber(b) }
    if (/^divide\s+(.+)\s+by\s+(.+)$/i.test(m)) { const [, a, b] = m.match(/^divide\s+(.+)\s+by\s+(.+)$/i); return toNumber(a) / toNumber(b) }
    const js = m.replace(/sin\(/g, "Math.sin(").replace(/cos\(/g, "Math.cos(").replace(/tan\(/g, "Math.tan(").replace(/log\(/g, "Math.log10(").replace(/ln\(/g, "Math.log(").replace(/sqrt\(/g, "Math.sqrt(").replace(/pow\(/g, "Math.pow(")
    return Function(`return (${js})`)()
  } catch (e) { return "Error" }
}
function convert(n, f, t) { if (f === "m" && t === "km") return n / 1000; if (f === "km" && t === "m") return n * 1000; if (f === "m" && t === "mi") return n / 1609.344; if (f === "mi" && t === "m") return n * 1609.344; if (f === "kg" && t === "lb") return n * 2.20462; if (f === "lb" && t === "kg") return n / 2.20462; if (f === "C" && t === "F") return n * 9 / 5 + 32; if (f === "F" && t === "C") return (n - 32) * 5 / 9; if (f === "C" && t === "K") return n + 273.15; if (f === "K" && t === "C") return n - 273.15; return "Error" }
function compute() { const r = evalMath(expr.value); output.textContent = String(r); pushHistory(`${expr.value} = ${r}`); codePreview.textContent = `system.output(${expr.value});`; hljs.highlightElement(codePreview) }
computeBtn.addEventListener("click", compute)
storeBtn.addEventListener("click", () => { memStore.last = expr.value; localStorage.setItem("calc.mem", JSON.stringify(memStore)) })
recallBtn.addEventListener("click", () => { if (memStore.last) { expr.value = memStore.last } })
clearBtn.addEventListener("click", () => { historyData = []; localStorage.setItem("calc.history", "[]"); renderHistory() })
expr.addEventListener("keydown", e => { if (e.key === "Enter") compute(); if (e.ctrlKey && e.key.toLowerCase() === "m") storeBtn.click(); if (e.ctrlKey && e.key.toLowerCase() === "r") recallBtn.click(); if (e.ctrlKey && e.key.toLowerCase() === "h") { document.querySelector(".history").classList.toggle("hidden") } })
