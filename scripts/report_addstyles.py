#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从详细设计文档把 Heading 1/2 样式注入测试报告文档，保持格式一致。"""
import copy
import docx
from docx.oxml.ns import qn

SRC = "轻量时间敏感网络详细设计.docx"
DST = "轻量时间敏感网络测试报告.docx"

src = docx.Document(SRC)
dst = docx.Document(DST)

src_styles = src.styles.element
dst_styles = dst.styles.element

existing = {s.get(qn("w:styleId")) for s in dst_styles.findall(qn("w:style"))}
existing_names = set()
for s in dst_styles.findall(qn("w:style")):
    n = s.find(qn("w:name"))
    if n is not None:
        existing_names.add(n.get(qn("w:val")))
print("已有样式名:", existing_names)

# 目标文档已占用 styleId 2/3，故注入时重映射为不冲突的 id
remap = {"heading 1": "Heading1", "heading 2": "Heading2"}
for s in src_styles.findall(qn("w:style")):
    name_el = s.find(qn("w:name"))
    if name_el is None:
        continue
    nm = name_el.get(qn("w:val"))
    if nm in remap and nm not in existing_names:
        new_sid = remap[nm]
        if new_sid in existing:
            continue
        node = copy.deepcopy(s)
        node.set(qn("w:styleId"), new_sid)
        # basedOn / next 指向 Normal(styleId=1)，目标文档 Normal 也是 1，保持即可
        dst_styles.append(node)
        print("注入样式:", nm, "-> styleId=", new_sid)

dst.save(DST)
print("完成。可用样式:", [s.name for s in dst.styles])