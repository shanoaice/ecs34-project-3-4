#include "StringUtils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

using std::string;

namespace StringUtils {

std::string Slice(const std::string &str, ssize_t start, ssize_t end) noexcept {
    if (start >= static_cast<ssize_t>(str.length()))
        return {};
    auto iter_begin = start < 0 ? str.cend() + start : str.cbegin() + start;
    auto iter_end = end <= 0 ? str.cend() + end : str.cbegin() + end;
    return {iter_begin, iter_end};
}

std::string Capitalize(const std::string &str) noexcept {
    auto newstr = string(str);
    std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
    if (!newstr.empty()) {
        newstr[0] = std::toupper(newstr[0]);
    }
    return newstr;
}

std::string Upper(const std::string &str) noexcept {
    string newstr(str);
    auto iter_begin = newstr.begin();
    auto iter_end = newstr.end();
    while (iter_begin != iter_end) {
        *iter_begin = std::toupper(*iter_begin);
        iter_begin++;
    }
    return newstr;
}

std::string Lower(const std::string &str) noexcept {
    string newstr(str);
    auto iter_begin = newstr.begin();
    auto iter_end = newstr.end();
    while (iter_begin != iter_end) {
        *iter_begin = std::tolower(*iter_begin);
        ++iter_begin;
    }
    return newstr;
}

std::string LStrip(const std::string &str) noexcept {
    auto iter_base = std::find_if_not(str.cbegin(), str.cend(),
                                      [](auto ch) { return std::isspace(ch); });
    return {iter_base, str.cend()};
}

std::string RStrip(const std::string &str) noexcept {
    auto iter_back_base = std::find_if_not(
        str.crbegin(), str.crend(), [](auto ch) { return std::isspace(ch); });
    return {str.cbegin(), iter_back_base.base()};
}

std::string Strip(const std::string &str) noexcept {
    auto it_start = std::find_if_not(str.cbegin(), str.cend(),
                                     [](auto ch) { return std::isspace(ch); });
    auto it_end = std::find_if_not(str.crbegin(), str.crend(),
                                   [](auto ch) { return std::isspace(ch); });
    return {it_start, it_end.base()};
}

std::string Center(const std::string &str, int width, char fill) noexcept {
    if (width <= static_cast<int>(str.length()))
        return {str};
    auto front = (width - str.length()) / 2;
    auto back = width - str.length() - front;
    return string(front, fill) + str + string(back, fill);
}

std::string LJust(const std::string &str, int width, char fill) noexcept {
    if (width <= static_cast<int>(str.length()))
        return {str};
    return str + string(width - str.length(), fill);
}

std::string RJust(const std::string &str, int width, char fill) noexcept {
    if (width <= static_cast<int>(str.length()))
        return {str};
    return string(width - str.length(), fill) + str;
}

std::string Replace(const std::string &str, const std::string &old,
                    const std::string &rep) noexcept {
    if (old.empty()) {
        return {str};
    }
    string newstr(str);
    string::size_type pos = 0;
    while ((pos = newstr.find(old, pos)) != string::npos) {
        newstr.replace(pos, old.length(), rep);
        pos += rep.length();
    }
    return newstr;
}

std::vector<std::string> Split(const std::string &str,
                               const std::string &splt) noexcept {
    if (str.empty())
        return {};

    if (splt.empty()) {
        std::vector<std::string> out;

        auto pos = std::find_if_not(str.cbegin(), str.cend(),
                                    [](auto ch) { return isspace(ch); });
        while (true) {
            auto next = std::find_if(pos, str.cend(),
                                     [](auto ch) { return isspace(ch); });
            if (next == str.cend()) {
                out.emplace_back(pos, next);
                break;
            }
            out.emplace_back(pos, next);
            pos = std::find_if_not(next + 1, str.cend(),
                                   [](auto ch) { return isspace(ch); });
        }
        return out;
    }

    std::vector<std::string> out;

    std::size_t pos = 0;
    while (true) {
        std::size_t next = str.find(splt, pos);
        if (next == std::string::npos) {
            out.emplace_back(str.substr(pos));
            break;
        }
        out.emplace_back(str.substr(pos, next - pos));
        pos = next + splt.size();
    }
    return out;
}

std::string Join(const std::string &str,
                 const std::vector<std::string> &vect) noexcept {
    if (vect.empty())
        return "";

    std::ostringstream sstr;

    bool first = true;

    for (const auto &it : vect) {
        if (!first)
            sstr << str;
        sstr << it;
        first = false;
    }

    return sstr.str();
}

std::string ExpandTabs(const std::string &str, int tabsize) noexcept {
    if (tabsize <= 0)
        return Replace(str, "\t", "");
    std::string out;
    size_t pos = 0;
    for (const auto c : str) {
        if (c == '\t') {
            const size_t spaces = tabsize - (pos % tabsize);
            out.append(spaces, ' ');
            pos += spaces;
        } else {
            out += c;
            pos++;
        }
    }
    return out;
}

int min3(int a, int b, int c) { return std::min(std::min(a, b), c); }

int EditDistanceRec(const std::string &left, const std::string &right, size_t i,
                    size_t j) noexcept {
    if (i >= left.size())
        return right.size() - j;
    if (j >= right.size())
        return left.size() - i;
    if (left[i] == right[j])
        return EditDistanceRec(left, right, i + 1, j + 1);
    return 1 + min3(EditDistanceRec(left, right, i, j + 1),
                    EditDistanceRec(left, right, i + 1, j),
                    EditDistanceRec(left, right, i + 1, j + 1));
}

int EditDistance(const std::string &left, const std::string &right,
                 bool ignorecase) noexcept {
    return EditDistanceRec(ignorecase ? Upper(left) : left,
                           ignorecase ? Upper(right) : right, 0, 0);
}

bool IsAllWhiteSpace(const std::string &s) noexcept {
    return std::all_of(s.cbegin(), s.cend(),
                       [](char c) { return std::isspace(c); });
}

}; // namespace StringUtils
