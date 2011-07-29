/**
 * @author <a href="mailto:andre.anjos@idiap.ch">Andre Anjos</a> 
 *
 * @brief Combines all modules to make up the complete bindings
 */

#include <boost/python.hpp>

using namespace boost::python;

void bind_core_vectors();
void bind_core_arrayvectors_1();
void bind_core_arrayvectors_2();
void bind_core_arrayvectors_3();
void bind_core_arrayvectors_4();

BOOST_PYTHON_MODULE(libpytorch_core_vector) {
  docstring_options docopt; 
# if !defined(TORCH_DEBUG)
  docopt.disable_cpp_signatures();
# endif
  scope().attr("__doc__") = "Torch core classes and sub-classes for std::vector manipulation from python";
  bind_core_vectors();
  bind_core_arrayvectors_1();
  bind_core_arrayvectors_2();
  bind_core_arrayvectors_3();
  bind_core_arrayvectors_4();
}
