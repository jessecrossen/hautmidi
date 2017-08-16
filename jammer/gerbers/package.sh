#!/bin/bash

set -x -e

# OSH Park
rm jammer-osh.* || true
cp jammer.top.gbr          jammer-osh.GTL # Top Layer
cp jammer.bottom.gbr       jammer-osh.GBL # Bottom Layer
cp jammer.topmask.gbr      jammer-osh.GTS # Top Soldermask
cp jammer.bottommask.gbr   jammer-osh.GBS # Bottom Soldermask
cp jammer.topsilk.gbr      jammer-osh.GTO # Top Silkscreen
cp jammer.outline.gbr      jammer-osh.GKO # Board Outline
cp jammer.plated-drill.cnc jammer-osh.XLN # Drills
zip jammer-osh.zip jammer-osh.*
rm jammer-osh.[GX]*

# Advanced Circuits
rm jammer-ac.* || true
cp jammer.top.gbr          jammer-ac.GTL # Top Layer
cp jammer.bottom.gbr       jammer-ac.GBL # Bottom Layer
cp jammer.topmask.gbr      jammer-ac.GTS # Top Soldermask
cp jammer.bottommask.gbr   jammer-ac.GBS # Bottom Soldermask
cp jammer.topsilk.gbr      jammer-ac.GTO # Top Silkscreen
cp jammer.plated-drill.cnc jammer-ac.DRL # Drills
cp jammer.fab.gbr          jammer-ac.DD1 # Fab Drawing
zip jammer-ac.zip jammer-ac.*
rm jammer-ac.[GD]*
