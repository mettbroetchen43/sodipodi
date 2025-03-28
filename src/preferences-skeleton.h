#define __PREFERENCES_SKELETON_H__

static const char preferences_skeleton[] =
"<sodipodi version=\"" VERSION "\" xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\">"
"  <interface id=\"toolboxes\">"
"    <interface id=\"file\" state=\"1\"/>"
"    <interface id=\"edit\" state=\"1\"/>"
"    <interface id=\"object\" state=\"1\"/>"
"    <interface id=\"selection\" state=\"0\"/>"
"    <interface id=\"draw\" state=\"1\"/>"
"    <interface id=\"zoom\" state=\"0\"/>"
"    <interface id=\"node\" state=\"0\"/>"
"  </interface>"

"  <group id=\"documents\">"
"    <group id=\"recent\"/>"
"  </group>"

"  <group id=\"template\">"
"    <sodipodi:namedview id=\"base\"/>"
"  </group>"

"  <group id=\"tools\""
"         style=\"fill:gray;fill-opacity:1;"
"                 stroke:black;stroke-opacity:1;stroke-width:1pt;stroke-linejoin:miter;stroke-linecap:butt;\">"
"    <group id=\"shapes\" style=\"fill-rule:evenodd;stroke:none;\">"
"      <eventcontext id=\"rect\"/>"
"      <eventcontext id=\"arc\"/>"
"      <eventcontext id=\"star\" magnitude=\"5\"/>"
"      <eventcontext id=\"spiral\" style=\"fill:none;stroke:black;\"/>"
"    </group>"
"    <group id=\"freehand\""
"         style=\"fill:none;fill-rule:evenodd;stroke:black;stroke-opacity:1;stroke-width:1pt;stroke-linejoin:miter;stroke-linecap:butt;\">"
"      <eventcontext id=\"pencil\"/>"
"      <eventcontext id=\"pen\" mode=\"drag\"/>"
"    </group>"
"    <eventcontext id=\"calligraphic\" style=\"fill:black;fill-rule:nonzero;stroke:none;\""
"                       mass=\"0.3\" drag=\"0.5\" angle=\"30\" width=\"0.2\"/>"
"    <eventcontext id=\"text\""
"                  style=\"fill:black;stroke:none;font-family:helvetica;font-style:normal;font-weight:normal;font-size:12px;\""
"                  xml:space=\"preserve\"/>"
"    <eventcontext id=\"nodes\"/>"
"    <eventcontext id=\"zoom\"/>"
"    <eventcontext id=\"select\">"
"      <function id=\"cycling\" scope=\"global\" focus=\"preserve\"/>"
"    </eventcontext>"
"  </group>"

"  <group id=\"palette\">"
"    <group id=\"dashes\">"
"      <dash id=\"solid\" style=\"stroke-dasharray:none;\"/>"
"      <dash id=\"dash-1-1\" style=\"stroke-dasharray:1 1;\"/>"
"      <dash id=\"dash-1-2\" style=\"stroke-dasharray:1 2;\"/>"
"      <dash id=\"dash-1-4\" style=\"stroke-dasharray:1 4;\"/>"
"      <dash id=\"dash-2-1\" style=\"stroke-dasharray:2 1;\"/>"
"      <dash id=\"dash-4-1\" style=\"stroke-dasharray:4 1;\"/>"
"      <dash id=\"dash-2-2\" style=\"stroke-dasharray:2 2;\"/>"
"      <dash id=\"dash-4-4\" style=\"stroke-dasharray:4 4;\"/>"
"      <dash id=\"dash-4-2-1-2\" style=\"stroke-dasharray:4 2 1 2;\"/>"
"    </group>"
"  </group>"

"  <group id=\"printing\">"
"    <settings id=\"ps\"/>"
"    <settings id=\"plain\"/>"
"  </group>"
"</sodipodi>";

#define PREFERENCES_SKELETON_SIZE (sizeof (preferences_skeleton) - 1)
