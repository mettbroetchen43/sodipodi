#!/bin/sh

xgettext --default-domain=sodipodi --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f sodipodi.po \
   || ( rm -f ./sodipodi.pot \
    && mv sodipodi.po ./sodipodi.pot )
