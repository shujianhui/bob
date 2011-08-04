/**
 * @author <a href="mailto:andre.dos.anjos@gmail.com">Andre Anjos</a> 
 * @date Wed  6 Apr 19:41:27 2011 
 *
 * @brief A bunch of private utilities to make programming against the HDF5
 * library a little bit more confortable.
 *
 * Classes and non-member methods in this file handle the low-level HDF5 C-API
 * and try to make it a little bit safer and higher-level for use by the
 * publicly visible HDF5File class. The functionality here is heavily based on
 * boost::shared_ptr's for handling automatic deletion and releasing of HDF5
 * objects. Two top-level classes do the whole work: File and Dataset. The File
 * class represents a raw HDF5 file. You can iterate with it in a very limited
 * way: create one, rename an object or delete one. The Dataset object
 * encapsulates reading and writing of data from a specific HDF5 dataset.
 * Everything is handled automatically and the user should not have to worry
 * about it too much. 
 *
 * @todo Missing support for std::string, list<std::string>
 * @todo Missing support for attributes
 * @todo Missing support for arbitrary groups (80% done see TODOs)
 * @todo Inprint file creation time, author, comments?
 * @todo Missing support for automatic endianness conversion
 * @todo Missing true support for scalars
 */

#ifndef TORCH_IO_HDF5UTILS_H 
#define TORCH_IO_HDF5UTILS_H

#include <map>
#include <vector>

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <blitz/array.h>
#include <blitz/tinyvec-et.h>
#include <hdf5.h>

#include "core/Exception.h"
#include "core/array_assert.h"

#include "io/HDF5Exception.h"
#include "io/HDF5Types.h"

/**
 * Checks if the version of HDF5 installed is greater or equal to some set of
 * values. (extracted from hdf5-1.8.7)
 */
#ifndef H5_VERSION_GE
#define H5_VERSION_GE(Maj,Min,Rel) \
 (((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR==Min) && (H5_VERS_RELEASE>=Rel)) || \
  ((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR>Min)) || \
  (H5_VERS_MAJOR>Maj))
#endif

namespace Torch { namespace io { namespace detail { namespace hdf5 {

  //Forward declaration for File
  class Dataset;

  /**
   * An HDF5 C-style file that knows how to close itself.
   */
  class File {

    public:

      /**
       * Creates a new HDF5 file. Optionally set the userblock size (multiple
       * of 2 number of bytes).
       */
      File(const boost::filesystem::path& path, unsigned int flags,
          size_t userblock_size=0);

      /**
       * Destructor virtualization
       */
      virtual ~File();

      /**
       * Unlinks a particular dataset from the file. Please note that this will
       * not erase the data on the current file as that functionality is not
       * provided by HDF5. To actually reclaim the space occupied by the
       * unlinked structure, you must re-save this file to another file. The
       * new file will not contain the data of any dangling datasets (datasets
       * w/o names or links).
       */
      void unlink (const std::string& path);

      /**
       * Renames a given dataset or group. Creates intermediary groups if
       * necessary.
       */
      void rename (const std::string& from,
          const std::string& to);

      /**
       * Returns the userblock size
       */
      size_t userblock_size() const;

    private: //not implemented

      File(const File& other);

      File& operator= (const File& other);

    public: //representation

      const boost::filesystem::path m_path; ///< path to the file
      unsigned int m_flags; ///< flags used to open it
      boost::shared_ptr<hid_t> m_fcpl; ///< file creation property lists
      boost::shared_ptr<hid_t> m_id; ///< the HDF5 id attributed to this file.
  };

  /**
   * An HDF5 C-style dataset that knows how to close itself.
   */
  class Dataset {

    public:

      /**
       * Creates a new HDF5 dataset by reading its contents from a certain
       * file.
       */
      Dataset(boost::shared_ptr<File>& f, const std::string& path);

      /**
       * Creates a new HDF5 dataset from scratch and inserts it in the given
       * file. If the Dataset already exists on file and the types are
       * compatible, we attach to that type, otherwise, we raise an exception.
       *
       * If a new Dataset is to be created, you can also set if you would like
       * to have as a list and the compression level. Note these settings have
       * no effect if the Dataset already exists on file, in which case the
       * current settings for that dataset are respected. The maximum value for
       * the gzip compression is 9. The value of zero turns compression off
       * (the default).
       *
       * The effect of setting "list" to false is that the created dataset:
       *
       * a) Will not be expandible (chunked)
       * b) Will contain the exact number of dimensions of the input type.
       *
       * When you set "list" to true (the default), datasets are created with
       * chunking automatically enabled (the chunk size is set to the size of
       * the given variable) and an extra dimension is inserted to accomodate
       * list operations.
       */
      Dataset(boost::shared_ptr<File>& f, const std::string&, 
          const Torch::io::HDF5Type& type, bool list=true,
          size_t compression=0);

      /**
       * Destructor virtualization
       */
      virtual ~Dataset();

      /**
       * Returns the number of objects installed at this dataset from the
       * perspective of the default compatible type.
       */
      size_t size() const;

      /**
       * Returns the number of objects installed at this dataset from the
       * perspective of the default compatible type. If the given type is not
       * compatible, raises a type error.
       */
      size_t size(const Torch::io::HDF5Type& type) const;

      /**
       * DATA READING FUNCTIONALITY
       */

      /**
       * Reads data from the file into a scalar. The conditions bellow have to
       * be respected:
       *
       * a. My internal shape is 1D **OR** my internal shape is 2D, but the
       *    extent of the second dimension is 1.
       * b. The indexed position exists
       *
       * If the internal shape is not like defined above, raises a type error.
       * If the indexed position does not exist, raises an index error.
       */
      template <typename T> void read(size_t index, T& value) {
        Torch::io::HDF5Type dest_type(value);
        read(index, dest_type, reinterpret_cast<void*>(&value));
      }

      /**
       * Reads data from the file into a scalar (allocated internally). The
       * same conditions as for read(index, value) apply.
       */
      template <typename T> T read(size_t index) {
        T retval;
        read(index, retval);
        return retval;
      }

      /**
       * Reads data from the file into a scalar. This is equivalent to using
       * read(0, value). The same conditions as for read(index=0, value) apply.
       */
      template <typename T> void read(T& value) {
        read(0, value);
      }

      /**
       * Reads data from the file into a scalar. This is equivalent to using
       * read(0). The same conditions as for read(index=0, value) apply.
       */
      template <typename T> T read() {
        T retval;
        read(0, retval);
        return retval;
      }

      /**
       * Reads data from the file into a array. The following conditions have
       * to be respected:
       *
       * a. My internal shape is the same as the shape of the given value
       *    **OR** my internal shape has one more dimension as the given value.
       *    In this case, the first dimension of the internal shape is
       *    considered to be an index and the remaining shape values the
       *    dimension of the value to be read. The given array has to be
       *    compatible with this re-defined N-1 shape.
       * b. The indexed position exists
       *
       * If the internal shape is not like defined above, raises a type error.
       * If the index does not exist, raises an index error.
       *
       * @param index Which of the arrays to read in the current dataset, by
       * order
       * @param value The output array data will be stored inside this
       * variable. This variable has to be a zero-based C-style contiguous
       * storage array. If that is not the case, we will raise an exception.
       */
      template <typename T, int N> 
        void readArray(size_t index, blitz::Array<T,N>& value) {
          Torch::core::array::assertCZeroBaseContiguous(value);
          Torch::io::HDF5Type dest_type(value);
          read(index, dest_type, reinterpret_cast<void*>(value.data()));
        }

      /**
       * Reads data from the file into an array allocated dynamically. The same
       * conditions as for readArray(index, value) apply.
       *
       * @param index Which of the arrays to read in the current dataset, by
       * order
       */
      template <typename T, int N> 
        blitz::Array<T,N> readArray(size_t index) {
          for (size_t k=0; k<m_type.size(); ++k) {
            const Torch::io::HDF5Shape& S = boost::get<0>(m_type[k]).shape();
            if(S.n() == N) {
              blitz::TinyVector<int,N> shape;
              S.set(shape);
              blitz::Array<T,N> retval(shape);
              readArray(index, retval);
              return retval;
            }
          }
          throw Torch::io::HDF5IncompatibleIO(m_parent->m_path.string(), 
              m_path, boost::get<0>(m_type[0]).str(), "dynamic shape unknown");
        }

      /**
       * Reads data from the file into a array. This is equivalent to using
       * readArray(0, value). The same conditions as for readArray(index=0,
       * value) apply.
       *
       * @param index Which of the arrays to read in the current dataset, by
       * order
       * @param value The output array data will be stored inside this
       * variable. This variable has to be a zero-based C-style contiguous
       * storage array. If that is not the case, we will raise an exception.
       */
      template <typename T, int N> 
        void readArray(blitz::Array<T,N>& value) {
          readArray(0, value);
        }

      /**
       * Reads data from the file into a array. This is equivalent to using
       * readArray(0). The same conditions as for readArray(index=0, value)
       * apply.
       */
      template <typename T, int N> 
        blitz::Array<T,N> readArray() {
          return readArray<T,N>(0);
        }

      /**
       * DATA WRITING FUNCTIONALITY
       */

      /**
       * Modifies the value of a scalar inside the file. Modifying a value
       * requires that the expected internal shape for this dataset and the
       * shape of the given scalar are consistent. To replace a scalar the
       * conditions bellow have to be respected:
       *
       * a. The internal shape is 1D **OR** the internal shape is 2D, but the
       *    second dimension of the internal shape has is extent == 1.
       * b. The given indexing position exists
       *
       * If the above conditions are not met, an exception is raised.
       */
      template <typename T> void replace(size_t index, const T& value) {
        Torch::io::HDF5Type dest_type(value);
        write(index, dest_type, reinterpret_cast<const void*>(&value));
      }

      /**
       * Modifies the value of a scalar inside the file. This is equivalent to
       * using replace(0, value). The same conditions as for replace(index=0,
       * value) apply. 
       */
      template <typename T> void replace(const T& value) {
        replace(0, value);
      }

      /**
       * Inserts a scalar in the current (existing ;-) dataset. This will
       * trigger writing data to the file. Adding a scalar value requires that
       * the expected internal shape for this dataset and the shape of the
       * given scalar are consistent. To add a scalar the conditions
       * bellow have to be respected:
       *
       * a. The internal shape is 1D **OR** the internal shape is 2D, but the
       *    second dimension of the internal shape has is extent == 1.
       * b. This dataset is expandible (chunked)
       *
       * If the above conditions are not met, an exception is raised.
       */
      template <typename T> void add(const T& value) {
        Torch::io::HDF5Type dest_type(value);
        extend(dest_type, reinterpret_cast<const void*>(&value));
      }

      /**
       * Replaces data at the file using a new array. Replacing an existing
       * array requires shape consistence. The following conditions should be
       * met:
       *
       * a. My internal shape is the same as the shape of the given value
       *    **OR** my internal shape has one more dimension as the given value.
       *    In this case, the first dimension of the internal shape is
       *    considered to be an index and the remaining shape values the
       *    dimension of the value to be read. The given array has to be
       *    compatible with this re-defined N-1 shape.
       * b. The given indexing position exists.
       *
       * If the internal shape is not like defined above, raises a type error.
       * If the indexed position does not exist, raises an index error.
       *
       * @param index Which of the arrays to read in the current dataset, by
       * order
       * @param value The output array data will be stored inside this
       * variable. This variable has to be a zero-based C-style contiguous
       * storage array. If that is not the case, we will raise an exception.
       */
      template <typename T, int N> 
        void replaceArray(size_t index, const blitz::Array<T,N>& value) {
          Torch::io::HDF5Type dest_type(value);
          if(!Torch::core::array::isCZeroBaseContiguous(value)) {
            blitz::Array<T,N> tmp = value.copy();
            write(index, dest_type, reinterpret_cast<const void*>(tmp.data()));
          }
          else {
            write(index, dest_type,
                reinterpret_cast<const void*>(value.data()));
          }
        }

      /**
       * Replaces data at the file using a new array. This is equivalent to
       * calling replaceArray(0, value). The conditions for
       * replaceArray(index=0, value) apply.
       *
       * @param value The output array data will be stored inside this
       * variable. This variable has to be a zero-based C-style contiguous
       * storage array. If that is not the case, we will raise an exception.
       */
      template <typename T, int N> 
        void replaceArray(const blitz::Array<T,N>& value) {
          replaceArray(0, value);
        }

      /**
       * Appends a array in a certain subdirectory of the file. If that
       * subdirectory (or a "group" in HDF5 parlance) does not exist, it is
       * created. If the dataset does not exist, it is created, otherwise, we
       * append to it. In this case, the dimensionality of the scalar has to be
       * compatible with the existing dataset shape (or "dataspace" in HDF5
       * parlance). If you want to do this, first unlink and than use one of
       * the add() methods.
       */
      template <typename T, int N> 
        void addArray(const blitz::Array<T,N>& value) {
          Torch::io::HDF5Type dest_type(value);
          if(!Torch::core::array::isCZeroBaseContiguous(value)) {
            blitz::Array<T,N> tmp = value.copy();
            extend(dest_type, reinterpret_cast<const void*>(tmp.data()));
          }
          else {
            extend(dest_type, reinterpret_cast<const void*>(value.data()));
          }
      }

    private: //not implemented

      Dataset(const Dataset& other);

      Dataset& operator= (const Dataset& other);

    public: //part of the API
      
      /**
       * This is the type compatibility vector. Each object contains a boost
       * tuple with the following elements: 
       *
       * 1. The type object that represents compatibility in this mode
       * 2. The number of objects inside this dataset, would the user decide to
       *    read/write using this type
       * 3. If under these conditions, the type is chunked as expected and more
       *    atomic objects can be added.
       * 4. A shape object that helps the hyperslab read/write obj. offset
       * 5. A shape object that helps the hyperslab read/write obj. counting
       * 6. A pointer to a pre-allocated, compatible, memory space for data
       *    transfers
       */
      typedef boost::tuple<Torch::io::HDF5Type, 
                           size_t, 
                           bool,
                           Torch::io::HDF5Shape,
                           Torch::io::HDF5Shape, 
                           boost::shared_ptr<hid_t> > type_t;

    private: //some tricks

      /**
       * Selects a bit of the file to be affected at the next read or write
       * operation. This method encapsulate calls to H5Sselect_hyperslab().
       *
       * The index is checked for existence as well as the consistence of the
       * destination type.
       */
      std::vector<type_t>::iterator select (size_t index,
          const Torch::io::HDF5Type& dest);

      /**
       * Reads a previously selected area into the given (user) buffer.
       */
      void read (size_t index, const Torch::io::HDF5Type& dest, void* buffer);

      /**
       * Writes the contents of a given buffer into the file. The area that the
       * data will occupy should have been selected beforehand.
       */
      void write (size_t index, const Torch::io::HDF5Type& dest, 
          const void* buffer);

      /**
       * Extend the dataset with one extra variable.
       */
      void extend (const Torch::io::HDF5Type& dest, const void* buffer);

    public: //representation
  
      boost::shared_ptr<File> m_parent; ///< my parent file
      std::string m_path; ///< full path to this object
      boost::shared_ptr<hid_t> m_id; ///< the HDF5 Dataset this type points to
      boost::shared_ptr<hid_t> m_dt; ///< the datatype of this Dataset
      boost::shared_ptr<hid_t> m_filespace; ///< the "file" space for this set
      std::vector<type_t> m_type;
  };

  /**
   * Scans the input file, fills up a dictionary indicating
   * location/pointer to Dataset capable of reading that location.
   */
  void index(boost::shared_ptr<File>& file,
      std::map<std::string, boost::shared_ptr<Dataset> >& index);

}}}}

#endif /* TORCH_IO_HDF5UTILS_H */
