#define __EXTENSIONS_SKELETON_H__

static const char extensions_skeleton[] =
"<sodipodi version=\"" VERSION "\">\n"
"  <group id=\"extensions\">\n"
"    <module id=\"xmleditor\" name=\"XML Editor\" action=\"yes\">\n"
"      <action id=\"xmldialog\" type=\"internal\" path=\"xmleditor\"/>"
"      <about>\n"
"        <text language=\"C\"><![CDATA[Sodipodi XML Editor\n"
"Written by Lauris Kaplinski and MenTaLguY]]>"
"        </text>"
"      </about>\n"
"    </module>\n"
"  </group>\n"
"</sodipodi>\n";

#define EXTENSIONS_SKELETON_SIZE (sizeof (extensions_skeleton) - 1)
