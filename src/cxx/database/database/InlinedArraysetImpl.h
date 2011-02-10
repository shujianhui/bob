/**
 * @file database/InlinedArraysetImpl.h>
 * @author <a href="mailto:andre.anjos@idiap.ch">Andre Anjos</a> 
 *
 * @brief A class that implements the polimorphic behaviour required when
 * reading and writing blitz arrays to disk or memory.
 */

#ifndef TORCH_DATABASE_INLINEDARRAYSETIMPL_H
#define TORCH_DATABASE_INLINEDARRAYSETIMPL_H

#include <map>
#include <list>
#include <boost/shared_ptr.hpp>

#include "core/array_common.h"
#include "database/Array.h"

namespace Torch { namespace database { namespace detail {

  /**
   * An implementation of the Arrayset type that holds its contents in memory.
   */
  class InlinedArraysetImpl {

    public:
     
      /**
       * Starts an empty Arrayset. An empty set does not contain any typing
       * information so, you can add any Array to it. Once that is done for the
       * first time, we make sure all other additions will conform to that.
       */
      InlinedArraysetImpl();

      /**
       * Starts a new Arrayset with begin and end iterators to
       * boost::shared_ptr<Torch::database::Array>'s or anything else that
       * overwrite() can accept.
       */
      template <typename T> InlinedArraysetImpl(T begin, T end) {
        for (T it = begin; it != end; ++it) overwrite(*it);
      }

      /**
       * Starts a new Arrayset with an STL conformant iterable container. This
       * can be for example std::vector<Array> or std::list<Array>, as you
       * wish.
       *
       * boost::shared_ptr<Torch::database::Array>'s or anything else that
       * overwrite() can accept.
       */
      template <typename T> InlinedArraysetImpl(const T& iterable) {
        for (typename T::const_iterator it = iterable.begin(); it != iterable.end(); ++it) overwrite(*it);
      }

      /**
       * Copy construct by getting an extra reference to somebodies' arrays.
       */
      InlinedArraysetImpl(const InlinedArraysetImpl& other);

      /**
       * Destroyes me
       */
      virtual ~InlinedArraysetImpl();

      /**
       * Copies the content of the other array and gets a reference to the
       * other arrayset's data.
       */
      InlinedArraysetImpl& operator= (const InlinedArraysetImpl& other);

      /**
       * Gets the table of contents for the current array in a map you have to
       * pass. The map key represents the array id while the pointee are
       * boost::shared_ptr to the arrays.
       */
      inline const std::map<size_t, boost::shared_ptr<Torch::database::Array> >& index() const { return m_index; }

      /**
       * Returns a list of arrays in the order they were inserted.
       */
      inline const std::list<boost::shared_ptr<Torch::database::Array> >& arrays() const { return m_array; }

      /**
       * Accesses a single array by their id
       */
      const Torch::database::Array& operator[] (size_t id) const;
      Torch::database::Array& operator[] (size_t id);

      /**
       * Accesses a single array by their id, but gets a shared_ptr<Array>
       * instead.
       */
      boost::shared_ptr<const Torch::database::Array> ptr (size_t id) const;
      boost::shared_ptr<Torch::database::Array> ptr (size_t id);

      /**
       * Adds a new array to the list I have. The id will be automatically
       * assigned if the Array has a temporary id of 0 (zero). Otherwise we
       * will check for the availability of ids and throw an exception in case
       * an id is already taken. Please note that by using the following
       * methods a new Array will be copy constructed from the array you give
       * me as input.
       */
      void add(boost::shared_ptr<const Torch::database::Array> array);
      void add(const Torch::database::Array& array);
     
      /**
       * This is a special version of the addArray() method that will take a
       * reference to the array you are manipulating instead of the copying.
       */
      void add(boost::shared_ptr<Torch::database::Array> array);

      /**
       * Adds a new array to the list I have, but does not check for the id. If
       * the id is set to 0 (zero), assign an available one, otherwise just
       * overwrite.
       */
      void overwrite(boost::shared_ptr<const Torch::database::Array> array);
      void overwrite(const Torch::database::Array& array);

      /**
       * This is a special version of the overwriteArray() method that will
       * take a reference to the array you are manipulating instead of the
       * copying.
       */
      void overwrite(boost::shared_ptr<Torch::database::Array> array);

      /**
       * Removes the array with a certain id. If the array does not exist, no
       * exception is thrown -- this operation is a noop.
       */
      void remove(size_t id);
      void remove(boost::shared_ptr<const Torch::database::Array> array);
      void remove(const Torch::database::Array& array);

      /**
       * Some informative methods
       */
      inline Torch::core::array::ElementType getElementType() const 
      { return m_elementtype; }
      inline size_t getNDim() const { return m_ndim; }
      inline const size_t* getShape() const { return m_shape; }
      inline size_t getNSamples() const { return m_array.size(); }

      /**
       * Gets the next free id
       */
      size_t getNextFreeId() const;

      /**
       * Consolidates the array ids by resetting the first array to have id =
       * 1, the second id = 2 and so on.
       */
      void consolidateIds();

    private: //checking and typing updating

      /**
       * Checks that the current Arrayset is compatible with the given Array.
       */ 
      void checkCompatibility (const Torch::database::Array& a) const;

      /**
       * Updates the internal typing information of this Arrayset, *iff* it is
       * uninitialized. Otherwise, this is a noop.
       */
      void updateTyping (const Torch::database::Array& a);

    private: //representation
      Torch::core::array::ElementType m_elementtype; ///< Elements' type
      size_t m_ndim; ///< The number of dimensions
      size_t m_shape[Torch::core::array::N_MAX_DIMENSIONS_ARRAY]; ///< The array shape
      std::list<boost::shared_ptr<Torch::database::Array> > m_array; ///< My array list
      std::map<size_t, boost::shared_ptr<Torch::database::Array> > m_index; ///< My index

  };

}}}

#endif /* TORCH_DATABASE_INLINEDARRAYSETIMPL_H */
