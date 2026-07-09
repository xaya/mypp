// Copyright (C) 2023-2026 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_STATEMENT_HPP
#define MYPP_STATEMENT_HPP

#include <mysql.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace mypp
{

/**
 * RAII wrapper and helper class for a prepared statement.
 */
class Statement
{

public:

  /**
   * A state the statement can be in.
   */
  enum class State
  {
    /** The statement is just initialised.  */
    INITIALISED,
    /** The statement has been prepared with a concrete SQL query.  */
    PREPARED,
    /** The statement has been queried, and results can be seen.  */
    QUERIED,
    /**
     * The statement has been executed/queried and all results (if any)
     * have been fetched already.
     */
    FINISHED,
  };

  /**
   * Value indicating that the batch size has not been determined yet.
   * Once the first bind call is made (scalar or batch), this is replaced
   * with the actual batch size.
   */
  static constexpr unsigned BATCH_UNDEFINED = static_cast<unsigned> (-1);

private:

  /** The associated MYSQL connection handle.  */
  MYSQL* const handle;

  /** The underlying MYSQL_STMT handle.  */
  MYSQL_STMT* stmt = nullptr;

  /** The state of this statement.  */
  State state;

  /** Number of input parameters in the prepared SQL.  */
  unsigned numParams;

  /**
   * The batch size for array binding.  If BATCH_UNDEFINED, the batch
   * size has not been set yet and the data arrays are sized for
   * single-row mode (one element per parameter).
   */
  unsigned batchSize = BATCH_UNDEFINED;

  /**
   * The parameter BIND structs.  They are used for input parameters before
   * the statement is executed, and then for output parameters afterwards.
   */
  std::vector<MYSQL_BIND> params;

  /**
   * For parameters that are integers, this holds a copy of the
   * data value and is the pointer that the MYSQL_BIND buffer points to.
   *
   * For parameters that are strings, this holds the length value.
   *
   * The type of long long int (instead of e.g. int64_t) is chosen to match
   * the MYSQL_TYPE_LONGLONG for the buffer, and is assumed to be large enough
   * to hold any kind of data we might want (such as int64_t).
   */
  std::vector<long long int> intParams;

  /**
   * For parameters that are string (TEXT or BLOB), this holds a copy
   * of the data and is the memory that the buffer points to.
   */
  std::vector<std::string> stringParams;

  /**
   * For string/blob batch bindings, this holds pointers to the data
   * of each row's string.  MySQL requires char** for column-wise
   * array binding of string parameters.
   */
  std::vector<const char*> stringPtrs;

  /**
   * For input parameters, the is-null mask for batch-binding columns
   * to some NULL values.
   *
   * For output parameters, whether or not they are null.
   */
  std::vector<my_bool> isNull;

  /** The result metadata, if the statement has been queried.  */
  MYSQL_RES* resMeta = nullptr;

  /** The result metadata column fields.  */
  std::vector<const MYSQL_FIELD*> resFields;

  /** Map of result column names to their indices.  */
  std::unordered_map<std::string, unsigned> columnsByName;

  /**
   * Initialises the statement.
   */
  void Init ();

  /**
   * Cleans up the statement.
   */
  void CleanUp ();

  /**
   * Resizes the params vector, and zeros it.
   */
  void ResizeParams (size_t num);

  /**
   * Returns the index of the named output column.
   */
  unsigned GetIndex (const std::string& col) const;

  /**
   * Common implementation for binding integer parameters (scalar and batch).
   */
  void BindIntImpl (unsigned num, const int64_t* data, size_t count);

  /**
   * Common implementation for binding string/blob parameters
   * (scalar and batch).
   */
  void BindStrImpl (unsigned num, const std::string* data, size_t count,
                    enum_field_types type);

public:

  /**
   * Initialises the statement for a given database connection.
   */
  Statement (MYSQL* h);

  /**
   * Frees the internal MYSQL_STMT handle and cleans up everything.
   */
  ~Statement ();

  /**
   * Returns the current state.
   */
  inline State
  GetState () const
  {
    return state;
  }

  /**
   * Returns the current batch size, or BATCH_UNDEFINED if it has not been
   * set yet.
   */
  inline unsigned
  GetBatchSize () const
  {
    return batchSize;
  }

  /**
   * Returns the underlying MySQL handle.  Must only be used if the statement
   * has been prepared alrady.
   */
  MYSQL_STMT* operator* ();

  /**
   * Prepares the statement with an SQL string.  The number of parameters
   * to bind must be specified explicitly, too.
   *
   * If the statement is in "finished" state, another call to Prepare can
   * be done to "reset" it and make it reusable.
   */
  void Prepare (unsigned numParams, const std::string& sql);

  /**
   * Resets the statement back to the state after initially being prepared,
   * with all bindings cleared as well.  New parameters can be bound, and then
   * the statement can be re-executed without re-preparing the SQL string.
   */
  void Reset ();

  /**
   * Returns the raw MYSQL_BIND struct for the given parameter.
   */
  MYSQL_BIND* BindRaw (unsigned num);

  /**
   * Binds the given parameter to NULL.  If we already have a batch
   * of parameters (from a previous bind), this binds all values in the
   * column to NULL.
   */
  void BindNull (unsigned num);

  /**
   * Binds the given parameter to an array of NULL values, with a mask
   * specifying which rows are null and which are not (batch mode).
   *
   * Before this can be applied to a column, it has to be bound to some
   * values that set the column type.  Then this method can be applied
   * to mask out values that should be NULL.
   */
  void BindNull (unsigned num, const std::vector<bool>& nullMask);

  /**
   * Binds the given parameter to a type (such as int64_t or std::string).
   * This can also be called with std::vector of integers or strings,
   * in which case it starts or continues batch binding.
   */
  template <typename T>
    void Bind (unsigned num, const T& val);

  /**
   * Binds the given parameter to a BLOB.
   */
  void BindBlob (unsigned num, const std::string& val);

  /**
   * Binds the given parameter to an array of BLOBs (batch mode).
   */
  void BindBlob (unsigned num, const std::vector<std::string>& vals);

  /**
   * Executes the statement, not expecting a result (e.g. an UPDATE).
   */
  void Execute ();

  /**
   * Executes the statement, expecting a result (i.e. a SELECT).
   */
  void Query ();

  /**
   * Fetches the next result row.  Returns false if no more is available.
   */
  bool Fetch ();

  /**
   * Checks if the given output column of the current result row is null.
   */
  bool IsNull (const std::string& col) const;

  /**
   * Returns the value of the given output column in the current result row.
   * It must not be null and the type must match.
   */
  template <typename T>
    T Get (const std::string& col) const;

  /**
   * Returns the value of the given output column as BLOB.
   */
  std::string GetBlob (const std::string& col) const;

};

} // namespace mypp

#endif // MYPP_STATEMENT_HPP
