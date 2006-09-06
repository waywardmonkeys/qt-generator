/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <fstream>
#include "pp.h"

using namespace rpp;

#ifndef GCC_MACHINE
#  define GCC_MACHINE "i386-redhat-linux"
#endif

#ifndef GCC_VERSION
#  define GCC_VERSION "4.1.1"
#endif

void usage ()
{
  std::cerr << "usage: rpp file.cpp" << std::endl;
  ::exit (EXIT_FAILURE);
}

void dump_macros (pp_environment &env, pp &, std::ostream &__out)
{
  for (pp_environment::const_iterator it = env.first_macro (); it != env.last_macro (); ++it)
    {
      pp_macro const *m = *it;

      if (m->hidden)
        continue;

      std::string id (m->name->begin (), m->name->end ());
      __out << "#define " << id;

      if (m->function_like)
        {
          __out << "(";

          for (std::size_t i = 0; i < m->formals.size (); ++i)
            {
              if (i != 0)
                __out << ", ";

              pp_fast_string const *f = m->formals [i];
              std::string name (f->begin (), f->end ());
              __out << name;
            }

          if (m->variadics)
            __out << "...";

          __out << ")";
        }

      __out << "\t";
      if (m->definition)
        {
          std::string def (m->definition->begin (), m->definition->end ());
          __out << def;
        }

      __out << std::endl;
    }
}

int main (int, char *argv [])
{
  char const *input_file = 0;
  char const *output_file = 0;
  char const *include_pch_file = 0;
  bool opt_help = false;
  bool opt_dump_macros = false;
  bool opt_pch = false;

  pp_environment env;
  pp preprocess(env);

  std::string result;
  result.reserve (20 * 1024); // 20K

  pp_output_iterator<std::string> out (result);
  pp_null_output_iterator null_out;

  preprocess.push_include_path ("/usr/include");
  preprocess.push_include_path ("/usr/lib/gcc/" GCC_MACHINE "/" GCC_VERSION "/include");

  preprocess.push_include_path ("/usr/include/c++/" GCC_VERSION);
  preprocess.push_include_path ("/usr/include/c++/" GCC_VERSION "/" GCC_MACHINE);

  std::string extra_args;

  while (const char *arg = *++argv)
    {
      if (arg [0] != '-')
        input_file = arg;

      else if (! strcmp (arg, "-help"))
        opt_help = true;

      else if (! strcmp (arg, "-dM"))
        opt_dump_macros = true;

      else if (! strcmp (arg, "-pch"))
        opt_pch = true;

      else if (! strcmp (arg, "-msse"))
	{
	  pp_macro __macro;
	  __macro.name = pp_symbol::get ("__SSE__", 7);
	  env.bind (__macro.name, __macro);

	  __macro.name = pp_symbol::get ("__MMX__", 7);
	  env.bind (__macro.name, __macro);
	}

      else if (! strcmp (arg, "-include"))
        {
          if (argv [1])
	    include_pch_file = *++argv;
        }

      else if (! strncmp (arg, "-o", 2))
        {
          arg += 2;

          if (! arg [0] && argv [1])
            arg = *++argv;

          if (arg)
            output_file = arg;
        }

      else if (! strncmp (arg, "-conf", 8))
        {
          if (argv [1])
            preprocess.file (*++argv, null_out);
        }

      else if (! strncmp (arg, "-I", 2))
        {
          arg += 2;

          if (! arg [0] && argv [1])
            arg = *++argv;

          if (arg)
            preprocess.push_include_path (arg);
        }

      else if (! strncmp (arg, "-U", 2))
        {
          arg += 2;

          if (! arg [0] && argv [1])
            arg = *++argv;

          if (arg)
            {
              env.unbind (arg, strlen (arg));
            }
        }

      else if (! strncmp (arg, "-D", 2))
        {
          arg += 2;

          if (! arg [0] && argv [1])
            arg = *++argv;

          if (arg)
            {
              pp_macro __macro;

              char const *end = arg;
              char const *eq = 0;

              for (; *end; ++end)
                {
                  if (*end == '=')
                    eq = end;
                }

              if (eq != 0)
                {
                  __macro.name = pp_symbol::get (arg, eq - arg);
                  __macro.definition = pp_symbol::get (eq + 1, end - (eq + 1));
                }

              else
                {
                  __macro.name = pp_symbol::get (arg, end - arg);
                  __macro.definition = 0;
                }

              env.bind (__macro.name, __macro);
            }
        }
      else
        {
          extra_args += " ";
          extra_args += arg;
        }
    }

  if (! input_file || opt_help)
    {
      usage ();
      return EXIT_FAILURE;
    }

  std::string __ifile (input_file);
  bool is_c_file = false;
  if (__ifile.size () > 2 && __ifile [__ifile.size () - 1] == 'c' && __ifile [__ifile.size () - 2] == '.')
    {
      is_c_file = true;
      env.unbind ("__cplusplus", 11);

      pp_macro __macro;
      __macro.name = pp_symbol::get ("__null");
      __macro.definition = pp_symbol::get ("((void*) 0)");
      env.bind (__macro.name, __macro);

      // turn off the pch
      include_pch_file = 0;
    }
  else if (include_pch_file)
    {
      std::string __pch (include_pch_file);
      __pch += ".gch/c++.conf";

      //std::cerr << "*** pch file " << __pch << std::endl;
      preprocess.file (__pch, null_out);
    }

  if (opt_dump_macros)
    {
      preprocess.file (input_file, null_out);
      dump_macros (env, preprocess, std::cout);
      return EXIT_SUCCESS;
    }

  preprocess.file (input_file, out);

  if (opt_pch)
    {
      if (! output_file)
        {
          std::cerr << "*** WARNING expected a file name" << std::endl;
          return EXIT_FAILURE;
        }

      std::string __conf_file (output_file);
      __conf_file += ".conf";

      std::ofstream __out;
      __out.open (__conf_file.c_str ());
      dump_macros (env, preprocess, __out);
      __out.close ();

      std::string __pp_file (output_file);
      __pp_file += ".i";

      __out.open (__pp_file.c_str ());
      __out.write (result.c_str (), result.size ());
      __out.close ();
      return EXIT_SUCCESS;
    }

  std::ostream *__out = &std::cout;
  std::ofstream __ofile;

  if (output_file)
    {
      std::string __output_file_name (output_file);
      __ofile.open (output_file);
      __out = &__ofile;
    }

  if (include_pch_file)
    {
      std::string __pch (include_pch_file);
      __pch += ".gch/c++.i";

      std::ifstream __in (__pch.c_str ());

      char buffer [1024];
      while (__in.read (buffer, 1024))
        __out->write (buffer, 1024);

      __in.close ();
    }

  __out->write (result.c_str (), result.size ());

  if (output_file)
    __ofile.close ();

  return EXIT_SUCCESS;
}

// kate: space-indent on; indent-width 2; replace-tabs on;

