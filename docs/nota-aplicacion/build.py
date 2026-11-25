"""Genera el PDF de la nota de aplicación a partir de NOTA_APLICACION.md.
Uso: python3 build.py   (requiere pandoc y playwright/chromium)"""
import re, subprocess, pathlib, html

HERE = pathlib.Path(__file__).resolve().parent
DOC_ID = "MP3-AN-001"
DATE = "Noviembre 2026"
SHORT = "Reproductor MP3 portátil sin distracciones"

TABLE_CAPTIONS = [
    "Especificaciones clave del sistema",
    "Asignación de pines del ESP32-S3",
    "Componentes destacados",
    "Disipación del regulador AP2112K-3.3",
    "Presupuesto de potencia en +3V3 (estimado)",
    "Autonomía y tiempo de carga estimados",
    "Función de los botones en la Rev A",
    "Evolución del diseño",
    "Tareas FreeRTOS del firmware",
    "Plataforma de la PoC frente a la PCB Rev A",
    "Resultados de la prueba de concepto",
    "Hojas del esquemático",
    "Lista de materiales",
    "Reglas de diseño de la PCB",
    "Puntos de prueba",
]

md = (HERE / "NOTA_APLICACION.md").read_text(encoding="utf-8")
title = md.splitlines()[0].lstrip("# ").strip()
cover_md = md.split("<!-- cover-start -->")[1].split("<!-- cover-end -->")[0]
body_md = md.split("<!-- cover-end -->")[1]

def pandoc(text):
    return subprocess.run(["pandoc", "-f", "markdown", "-t", "html", "--mathml"],
                          input=text, capture_output=True, text=True, check=True).stdout

# ---------- portada: dos columnas ----------
parts = re.split(r"^## ", cover_md, flags=re.M)
sec = {p.split("\n", 1)[0].strip(): p.split("\n", 1)[1] for p in parts if p.strip()}
left = "".join(f"<h3>{k}</h3>" + pandoc(sec[k]) for k in ["Descripción", "Recursos"])
right = "".join(f"<h3>{k}</h3>" + pandoc(sec[k]) for k in ["Características", "Aplicaciones"])
doc_label, main_title = title.split(":", 1)
cover = f"""
<section class="cover">
  <div class="doclabel">{html.escape(doc_label.strip())}</div>
  <div class="covertitle">{html.escape(main_title.strip())}</div>
  <div class="coverrule"></div>
  <div class="cols"><div class="col">{left}</div><div class="col">{right}</div></div>
  <div class="coverfigs">
    <img src="img/blocks.png" alt="Diagrama de bloques">
    <img src="img/pcb_3d_top.png" alt="Render 3D de la PCB">
  </div>
</section>"""

# ---------- cuerpo ----------
body = pandoc(body_md)
# leyendas de tablas
ti = iter(TABLE_CAPTIONS)
def add_caption(m):
    cap = next(ti, None)
    return m.group(0) + (f"<caption>{cap}</caption>" if cap else "")
body = re.sub(r"<table[^>]*>", add_caption, body)
# tabla de contenido
toc_items = re.findall(r'<h([12]) id="([^"]+)">(.*?)</h\1>', body)
toc = '<section class="toc"><h1 class="tochead">Contenido</h1><ul>' + "".join(
    f'<li class="l{lvl}"><a href="#{i}">{t}</a></li>' for lvl, i, t in toc_items) + "</ul></section>"

css = (HERE / "style.css").read_text(encoding="utf-8")
page = f"""<!DOCTYPE html><html lang="es"><head><meta charset="utf-8"><title>{html.escape(title)}</title>
<style>{css}</style></head><body>{cover}{toc}<main>{body}</main></body></html>"""
(HERE / "nota.html").write_text(page, encoding="utf-8")

hdr = f"""<div style="font-family:Arial,sans-serif;font-size:7.5px;width:100%;margin:0 16mm;
 display:flex;justify-content:space-between;border-bottom:0.5px solid #000;padding-bottom:2px;color:#000">
 <span><i>Nota de aplicación {DOC_ID}</i></span><span>github.com/macardenascu/MP3_Electronic_Design</span></div>"""
ftr = f"""<div style="font-family:Arial,sans-serif;font-size:7.5px;width:100%;margin:0 16mm;
 display:flex;justify-content:space-between;border-top:0.5px solid #000;padding-top:2px;color:#000">
 <span>{DOC_ID} – {DATE}</span><span><i>{SHORT}</i></span><span class="pageNumber"></span></div>"""

from playwright.sync_api import sync_playwright
with sync_playwright() as p:
    b = p.chromium.launch()
    pg = b.new_page()
    pg.goto((HERE / "nota.html").as_uri())
    pg.wait_for_timeout(800)
    pg.pdf(path=str(HERE / "Nota_de_Aplicacion_Reproductor_MP3.pdf"), format="A4", print_background=True,
           display_header_footer=True, header_template=hdr, footer_template=ftr,
           margin={"top": "20mm", "bottom": "18mm", "left": "16mm", "right": "16mm"})
    b.close()
print("PDF listo")
