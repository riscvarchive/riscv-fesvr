// See LICENSE for license details.

#ifndef _OPTION_PARSER_H
#define _OPTION_PARSER_H

#include <initializer_list>
#include <functional>
#include <string>

class option_parser_t
{
  struct option_t
  {
  const char chr;
  const std::string str;
  const int arg;
  const std::function<void(const char*)> func;
  };
 public:
  option_parser_t(std::initializer_list<option_t>&&);
  option_parser_t(std::initializer_list<option_t>&&, std::function<void(void)>);
  const char* const* parse(char**);

 private:
  const std::initializer_list<option_t>&& opts;
  std::function<void(void)> helpmsg;
  void error(const char* msg, const char* argv0, const char* arg);
};

#endif
