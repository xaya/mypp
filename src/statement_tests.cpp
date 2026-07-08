// Copyright (C) 2024-2026 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "statement.hpp"

#include "tempdb.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <cstdlib>

namespace mypp
{
namespace
{

/** Environment variable holding the connection URL for the temp db.  */
constexpr const char* ENV = "MYPP_TEST_TEMPDB";

/**
 * Gets the URL to use for the temp db.
 */
std::string
GetTempDbUrl ()
{
  const char* url = std::getenv (ENV);
  CHECK (url != nullptr)
      << "Please set the environment variable '" << ENV << "'"
      << " to a MySQL URL for use in tests";
  return url;
}

class StatementTests : public testing::Test
{

protected:

  TempDb db;

  StatementTests ()
    : db(GetTempDbUrl ())
  {
    db.Initialise ();
  }

};

TEST_F (StatementTests, BasicUpdateAndQuery)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `name` VARCHAR(64) NOT NULL
    )
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (2, R"(
    INSERT INTO `test`
      (`id`, `name`) VALUES
      (1, 'foo'),
      (?, ?)
  )");
  stmt.Bind<int64_t> (0, 42);
  stmt.Bind<std::string> (1, "bar");
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT *
      FROM `test`
      ORDER BY `id`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("id"), 1);
  EXPECT_EQ (stmt.Get<std::string> ("name"), "foo");
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("id"), 42);
  EXPECT_EQ (stmt.Get<std::string> ("name"), "bar");
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, ResetAndReusePreparedStatement)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `name` VARCHAR(64) NOT NULL
    );
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (2, R"(
    INSERT INTO `test`
      (`id`, `name`)
      VALUES (?, ?)
  )");

  stmt.Bind<int64_t> (0, 10);
  stmt.Bind<std::string> (1, "foo");
  stmt.Execute ();

  stmt.Reset ();
  stmt.Bind<int64_t> (0, 20);
  stmt.Bind<std::string> (1, "bar");
  stmt.Execute ();

  stmt.Prepare (1, R"(
    SELECT `name`
      FROM `test`
      WHERE `id` = ?
  )");

  stmt.Bind<int64_t> (0, 10);
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<std::string> ("name"), "foo");

  stmt.Reset ();
  stmt.Bind<int64_t> (0, 20);
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<std::string> ("name"), "bar");
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, Null)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `name` VARCHAR(64) NULL
    );
    INSERT INTO `test`
      (`id`, `name`) VALUES
      (1, 'foo'),
      (2, NULL),
      (3, 'bar');
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (0, R"(
    SELECT `name`
      FROM `test`
      ORDER BY `id`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_FALSE (stmt.IsNull ("name"));
  EXPECT_EQ (stmt.Get<std::string> ("name"), "foo");
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_TRUE (stmt.IsNull ("name"));
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_FALSE (stmt.IsNull ("name"));
  EXPECT_EQ (stmt.Get<std::string> ("name"), "bar");
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, IntTypes)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `small` TINYINT NOT NULL,
      `big` BIGINT NOT NULL
    )
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (2, R"(
    INSERT INTO `test`
      (`id`, `small`, `big`) VALUES
      (1, -5, ?),
      (2, 100, ?)
  )");
  stmt.Bind (0, std::numeric_limits<int64_t>::min ());
  stmt.Bind (1, std::numeric_limits<int64_t>::max ());
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT `small`, `big`
      FROM `test`
      ORDER BY `id`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("small"), -5);
  EXPECT_EQ (stmt.Get<int64_t> ("big"), std::numeric_limits<int64_t>::min ());
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("small"), 100);
  EXPECT_EQ (stmt.Get<int64_t> ("big"), std::numeric_limits<int64_t>::max ());
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, Bool)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `boolean` BOOL NULL
    )
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (3, R"(
    INSERT INTO `test`
      (`id`, `boolean`) VALUES
      (1, ?),
      (2, ?),
      (3, ?)
  )");
  stmt.Bind (0, true);
  stmt.Bind (1, false);
  stmt.BindNull (2);
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT `boolean`
      FROM `test`
      ORDER BY `id`
  )");
  stmt.Query ();

  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_FALSE (stmt.IsNull ("boolean"));
  EXPECT_TRUE (stmt.Get<bool> ("boolean"));

  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_FALSE (stmt.IsNull ("boolean"));
  EXPECT_FALSE (stmt.Get<bool> ("boolean"));

  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_TRUE (stmt.IsNull ("boolean"));

  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, Blob)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `data` BLOB NOT NULL
    )
  )");

  const std::string data("x\0\xFFy", 4);

  Statement stmt(*db.Get ());
  stmt.Prepare (1, R"(
    INSERT INTO `test`
      (`data`) VALUES (?)
  )");
  stmt.BindBlob (0, data);
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT `data` FROM `test`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.GetBlob ("data"), data);
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, Unicode)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `text` TEXT NOT NULL
    )
  )");

  const std::string value = u8"abcäöüßxzy";

  Statement stmt(*db.Get ());
  stmt.Prepare (1, R"(
    INSERT INTO `test`
      (`text`) VALUES (?)
  )");
  stmt.Bind (0, value);
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT `text` FROM `test`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<std::string> ("text"), value);
  EXPECT_FALSE (stmt.Fetch ());
}

TEST_F (StatementTests, BindBatch)
{
  db.Get ().Execute (R"(
    CREATE TABLE `test` (
      `id` INT NOT NULL PRIMARY KEY,
      `name` VARCHAR(64) NULL
    )
  )");

  Statement stmt(*db.Get ());
  stmt.Prepare (2, R"(
    INSERT INTO `test`
      (`id`, `name`) VALUES (?, ?)
  )");
  stmt.Bind<std::vector<int64_t>> (0, {1, 2, 3});
  stmt.Bind<std::vector<std::string>> (1, {"a", "b", "c"});
  stmt.BindNull (1, {false, true, false});
  stmt.Execute ();

  /* Running an empty batch is fine, too.  */
  stmt.Reset ();
  stmt.Bind<std::vector<int64_t>> (0, {});
  stmt.Bind<std::vector<std::string>> (1, {});
  stmt.Execute ();

  stmt.Prepare (0, R"(
    SELECT `id`, `name`
      FROM `test`
      ORDER BY `id`
  )");
  stmt.Query ();
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("id"), 1);
  EXPECT_EQ (stmt.Get<std::string> ("name"), "a");
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("id"), 2);
  EXPECT_TRUE (stmt.IsNull ("name"));
  ASSERT_TRUE (stmt.Fetch ());
  EXPECT_EQ (stmt.Get<int64_t> ("id"), 3);
  EXPECT_EQ (stmt.Get<std::string> ("name"), "c");
  EXPECT_FALSE (stmt.Fetch ());
}

} // anonymous namespace
} // namespace mypp
