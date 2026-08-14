#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import docx

DOC = "轻量时间敏感网络详细设计.docx"

d = docx.Document(DOC)
for i, p in enumerate(d.paragraphs):
    t = p.text.strip()
    style = p.style.name if p.style else "?"
    if style.startswith("Heading") or t in (
        "模块详细设计", "关键业务流程详细设计", "接口设计"):
        print(f"{i:3d} | {style:20s} | {t[:60]}")