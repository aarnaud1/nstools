// MIT License
//
// Copyright (c) 2019 Agenium Scale
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "backend_make.hpp"
#include "shell.hpp"
#include <ctime>
#include <iostream>
#include <ns2/fs.hpp>

namespace backend {

// ----------------------------------------------------------------------------

static void make_output_rule(rule_desc_t const &rule_desc, ns2::ofile_t &out,
                             make_t::type_t which) {
  if (which == make_t::GNU) {
    if (rule_desc.type == rule_desc_t::Phony || rule_desc.cmds.size() == 0) {
      out << ".PHONY: " << rule_desc.target << "\n";
    }
  }
  ns2::dir_file_t df = ns2::split_path(rule_desc.output);
  if (which == make_t::POSIX && df.first != "") {
    die("POSIX Makefile does not support paths in targets", rule_desc.cursor);
  }
  out << ns2::sanitize(rule_desc.target) << ":";
  for (size_t i = 0; i < rule_desc.deps.size(); i++) {
    out << " " << ns2::sanitize(rule_desc.deps[i]);
  }
  out << "\n";
  if (rule_desc.type != rule_desc_t::Phony && rule_desc.cmds.size() > 0 &&
      df.first != "") {
    std::string folder(shell::ify(df.first));
#ifdef NS2_IS_MSVC
    out << "\tif not " + folder + " md " + folder + "\n";
#else
    out << "\tmkdir -p " + folder + "\n";
#endif
  }
  for (size_t i = 0; i < rule_desc.cmds.size(); i++) {
    out << "\t" << ns2::replace(rule_desc.cmds[i], "$", "$$") << "\n";
  }
  out << "\n";
}

// ----------------------------------------------------------------------------

void make(rules_t const &rules, std::string const &Makefile,
          make_t::type_t which, std::string const &cmdline) {
  ns2::ofile_t out(Makefile);
  char buf[256];
  time_t now = time(NULL);
  strftime(buf, 256, "%Y/%m/%d %H:%M:%S", localtime(&now));
  out << "#\n"
      << "# File generated by nsconfig on " << buf << "\n"
      << "# Command line: " << cmdline << "\n"
      << "#\n\n";

  if (which == make_t::GNU) {
    out << "ifeq (4.0,$(firstword $(sort $(MAKE_VERSION) 4.0)))\n"
        << "MAKEFLAGS += -r\n"
        << "MAKEFLAGS += -R\n"
        << "endif\n\n";
  }

  if (which == make_t::POSIX) {
    out << ".POSIX:\n";
  }
  if (which == make_t::POSIX || which == make_t::GNU) {
    out << ".DEFAULT:\n";
  }
  out << ".SUFFIXES:\n\n";

  // First rule must be 'all' (if it exists), cf. nmake documentation
  // When typing nmake without argument on command line, the first target
  // is executed.
  rule_desc_t const *rd = rules.find_by_target("all");
  if (rd != NULL) {
    make_output_rule(*rd, out, which);
  }

  // Dump all rules
  for (rules_t::const_iterator it = rules.begin(); it != rules.end(); ++it) {
    if (it->first != "all" && it->first != "install" && it->first != "clean") {
      make_output_rule(it->second, out, which);
    }
  }

  // Last rules are install and clean
  rd = rules.find_by_target("install");
  if (rd != NULL) {
    make_output_rule(*rd, out, which);
  }
  rd = rules.find_by_target("clean");
  if (rd != NULL) {
    make_output_rule(*rd, out, which);
  }

  switch (which) {
  case make_t::POSIX:
    OUTPUT << "POSIX";
    break;
  case make_t::GNU:
    OUTPUT << "GNU";
    break;
  case make_t::MSVC:
    OUTPUT << "MSVC";
    break;
  }
  std::cout << " Makefile written to " << Makefile << std::endl;
}

// ----------------------------------------------------------------------------

} // namespace backend
