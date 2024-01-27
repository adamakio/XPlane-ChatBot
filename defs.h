/**
 * @file defs.h
 * @author lc
 * @brief Header file containing various definitions, macros, and function declarations that are used throughout the plugin
 * @version 0.1
 * @date 2023-09-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef DEFS_H
#define DEFS_H

#include <string>
#include <vector>

#ifdef __APPLE__ 
#include <OpenGL/gl.h>
#else
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// Plugin Info
#define PLUGIN_NAME "ChatBot"
#define PLUGIN_SIGNATURE "chat.bot" 
#define PLUGIN_DESCRIPTION "A ChatBot Plugin"

// Generic Macros
#define UNUSED(x) (void)(x)
#define FREE_MEMORY(x) if(x){delete x; x = nullptr;}
#define ALLOCATE_MEMORY(x,y) {x = new y;}
#define FLOAT_CAST(x) static_cast<float>(x)
#define DOUBLE_CAST(x) static_cast<double>(x)
#define INT_CAST(x) static_cast<int>(x)
#define UINT_CAST(x) static_cast<uint>(x)
#define ULONG_CAST(x) static_cast<ulong>(x)
#define SET_LOCAL(x) this->m_##x = x
#define STR(x) const_cast<char*>(x)
#define SAFE_FUNC_CALL(pointer,function) if(pointer){pointer->function;}
#define CONCAT_VECTOR(x,y) x.insert(std::end(x), std::begin(y), std::end(y))
#define TO_LOWER(s) std::transform(s.begin(), s.end(), s.begin(), ::tolower)

#if IBM
#define _USE_MATH_DEFINES // for C
#include <ctype.h>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

// The maximum length that WE support
#define PATH_LEN_MAX 2048

//Xplane macros and definitions
#define DeclareMenuID_(x) static const char* x  = #x ///< This macro definition declares menu ID (need this)
#define CreateMenuID_(x) const_cast<void*>(static_cast<const void *>(x)) ///< This macro casts a string to a void pointer. It is used for creating menu items.

// Generic Functions
double calc_distance_m(double lat1, double long1, double lat2, double long2);
double calc_distance_nm(double lat1, double long1, double lat2, double long2);
double calc_distance_3D(double x1, double y1, double z1, double x2, double y2, double z2);
void multMatrixVec4f(GLfloat dst[4], const GLfloat m[16], const GLfloat v[4]);
void modelview_to_window_coords(int out_w[2], const GLfloat in_mv[4], const GLfloat mv[16], const GLfloat pr[16], const GLint viewport[4]);

std::string get_plugin_path();
bool does_file_exist(std::string file);
bool does_directory_exist(std::string directory);
std::string get_dir_name_from_path(const std::string &utf8_path);
std::string native_to_utf8(const std::string& native);
std::string utf8_to_native(const std::string& utf8);
std::string base64_encode(const std::vector<char>& buf);

int set_bit(int num, int position);
bool get_bit(int num, int position);

#endif // DEFS_H
