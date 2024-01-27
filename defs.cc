/**
 * @file defs.cc
 * @author lc
 * @brief Implementation of utility functions and conversions used in the XPlaneChatBot plugin.
 * @details This file provides various utility functions for distance calculations, coordinate conversions,
 * file and directory existence checks, and text encoding conversions. These functions are essential
 * for handling diverse data and ensuring cross-platform compatibility within the XPlaneChatBot plugin.
 * @version 0.1
 * @date 2023-09-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "defs.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cmath>
#include <sstream>
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"

using namespace std;

/// Not used or required (for now)
double calc_distance_m(double lat1, double long1, double lat2, double long2)
{
    lat1 = lat1 * M_PI / 180;
    long1 = long1 * M_PI / 180;
    lat2 = lat2 * M_PI / 180;
    long2 = long2 * M_PI / 180;

    double rEarth = 6372797;

    double dlat = lat2 - lat1;
    double dlong = long2 - long1;

    double x1 = sin(dlat / 2);
    double x2 = cos(lat1);
    double x3 = cos(lat2);
    double x4 = sin(dlong / 2);

    double x5 = x1 * x1;
    double x6 = x2 * x3 * x4 * x4;

    double temp1 = x5 + x6;

    double y1 = sqrt(temp1);
    double y2 = sqrt(1.0 - temp1);

    double temp2 = 2 * atan2(y1, y2);

    double range_m = temp2 * rEarth;

    return range_m;
}

/* Also unused (But I may use it later). 
Has a lot of repeated code from calc_distance_m. May want to refactor that.
*/
double calc_distance_nm(double lat1, double long1, double lat2, double long2)
{
    lat1 = lat1 * M_PI / 180;
    long1 = long1 * M_PI / 180;
    lat2 = lat2 * M_PI / 180;
    long2 = long2 * M_PI / 180;

    double rEarth = 6372.797;

    double dlat = lat2 - lat1;
    double dlong = long2 - long1;

    double x1 = sin(dlat / 2);
    double x2 = cos(lat1);
    double x3 = cos(lat2);
    double x4 = sin(dlong / 2);

    double x5 = x1 * x1;
    double x6 = x2 * x3 * x4 * x4;

    double temp1 = x5 + x6;

    double y1 = sqrt(temp1);
    double y2 = sqrt(1.0 - temp1);

    double temp2 = 2 * atan2(y1, y2);

    double rangeKm = temp2 * rEarth;

    double CalcRange = rangeKm * 0.539957;

    return CalcRange;
}

/// Also unused (But I may use it later)
double calc_distance_3D(double x1, double y1, double z1, double x2, double y2, double z2)
{
    auto d = pow((pow((x2 - x1),2) + pow((y2 - y1),2) + pow((z2 - z1),2)),1/2);

    return  d;
}

void multMatrixVec4f(GLfloat dst[4], const GLfloat m[16], const GLfloat v[4])
{
    dst[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
    dst[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
    dst[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
    dst[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
}

void modelview_to_window_coords(int out_w[2], const GLfloat in_mv[4], const GLfloat mv[16], const GLfloat pr[16], const GLint viewport[4])
{
    GLfloat eye[4], ndc[4];
    multMatrixVec4f(eye, mv, in_mv);
    multMatrixVec4f(ndc, pr, eye);
    ndc[3] = 1.0f / ndc[3];
    ndc[0] *= ndc[3];
    ndc[1] *= ndc[3];

    out_w[0] = INT_CAST((ndc[0] * 0.5f + 0.5f) * viewport[2] + viewport[0]);
    out_w[1] = INT_CAST((ndc[1] * 0.5f + 0.5f) * viewport[3] + viewport[1]);
}

/// @brief Checks if file exists
/// @param file Path to file
/// @return True if exists, False otherwise
bool does_file_exist(std::string file)
{
    ifstream f(file);

    if(f.is_open())
    {
        f.close();
        return true;
    }

    return false;
}

/// @brief Checks if directory exists
/// @param directory Path to directory
/// @return True if exists, False otherwise
bool does_directory_exist(std::string directory)
{
    bool success = false;
    struct stat info;

    if(stat(directory.c_str(), &info) != 0)
        success = false;
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
        success = true;
    else
        success = false;

    return success;
}

/**
 * @brief Retrieves the path of the plugin and provides your XPlaneChatBot plugin 
 * with the means to determine its own directory location within the X-Plane directory structure. 
 * This information is vital for various operational aspects, including file access, resource loading, and debugging.
 * 
 * @return std::string System path to plugin directory
 */
std::string get_plugin_path() 
{
    char path[256];
    XPLMGetPluginInfo(XPLMGetMyID(),nullptr,path,nullptr,nullptr);
    XPLMExtractFileAndPath(path);

    std::string pluginPath(path);
    if(pluginPath.at(pluginPath.length() - 1) == '/' || pluginPath.at(pluginPath.length() - 1) == '\\')
    {
        pluginPath.pop_back();
    }

    pluginPath = pluginPath.substr(0, pluginPath.find_last_of("\\/"));
    pluginPath += XPLMGetDirectorySeparator();

    return pluginPath;
}

/// @brief Get the name of directory from path
/// @param utf8Path Full path
/// @return Path to directory
std::string get_dir_name_from_path(const std::string& utf8Path)
{
    std::string nativePath = utf8_to_native(utf8Path);
#if IBM
    char drive[_MAX_DRIVE];
    char dirname[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    _splitpath_s(nativePath.c_str(), drive, _MAX_DRIVE, dirname, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    std::string dir = dirname;
#else
    std::string dir = dirname(&nativePath[0]);
#endif
    return native_to_utf8(dir);
}


#ifdef _WIN32
std::string native_to_utf8(const std::string& native)
{
    wchar_t buf[PATH_LEN_MAX];
    char res[PATH_LEN_MAX];

    MultiByteToWideChar(CP_ACP, 0, native.c_str(), -1, buf, sizeof(buf));
    WideCharToMultiByte(CP_UTF8, 0, buf, -1, res, sizeof(res), nullptr, nullptr);
    return res;
}
#else
std::string native_to_utf8(const std::string& native)
{
    return native;
}
#endif

#ifdef _WIN32
std::string utf8_to_native(const std::string& utf8)
{
    wchar_t buf[PATH_LEN_MAX];
    char res[PATH_LEN_MAX];

    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buf, sizeof(buf));
    WideCharToMultiByte(CP_ACP, 0, buf, -1, res, sizeof(res), nullptr, nullptr);
    return res;
}
#else
std::string UTF8ToNative(const std::string& utf8) {
    return utf8;
}
#endif

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string base64_encode(const std::vector<char>& buf) {
    std::string base64;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (size_t idx = 0; idx < buf.size(); ++idx) {
        char_array_3[i++] = buf[idx];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                base64 += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            base64 += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            base64 += '=';
    }

    return base64;
}


/// @brief Set bit at position to be 1
/// @param num Binary number
/// @param position Position to set bit to 1
/// @return New binary number with bit at position set to 1
int set_bit(int num, int position)
{
    int mask = 1 << position;
    return num | mask;
}

/// @brief Get value of bit (0 or 1) at position
/// @param num Binary number
/// @param position Position to evaluate bit
/// @return Value of bit 
bool get_bit(int num, int position)
{
    bool bit = num & (1 << position);
    return bit;
}


// Question I had about native -> UTF-8 conversion:
// Suppose your plugin needs to read a configuration file with a path containing non-ASCII characters (e.g., accented characters). 
// Before opening and reading the file, you would use native_to_utf8() to convert the native file path to UTF-8 encoding. 
// This ensures that the file path is correctly represented and can be used for file operations, regardless of the system's native encoding.