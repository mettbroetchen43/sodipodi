#ifndef DIR_UTIL_H
#define DIR_UTIL_H

/*
 * path-util.h
 *
 * here are functions sp_relative_path & cousins
 * maybe they are already implemented in standard libs
 *
 */

const gchar * sp_relative_path_from_path (const gchar * path, const gchar * base);
const gchar * sp_filename_from_path (const gchar * path);
const gchar * sp_extension_from_path (const gchar * path);

#endif
