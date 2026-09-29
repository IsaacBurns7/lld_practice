#include "DB.hpp"
#include "tests.hpp"
#include <iostream>
#include <climits>
#include <string_view>

namespace {
int g_pass = 0, g_fail = 0;

void check(bool ok, std::string_view expr, std::string_view name, int line) {
    if (ok) { ++g_pass; return; }
    ++g_fail;
    std::cout << "  FAIL [" << name << "] line " << line << ": " << expr << '\n';
}
} // namespace

#define CHECK(name, expr) check((expr), #expr, name, __LINE__)
#define CHECK_EQ(name, opt, val) CHECK(name, (opt).has_value() && *(opt) == (val))
#define CHECK_NONE(name, opt) CHECK(name, !(opt).has_value())

bool test_L1() {
    g_pass = g_fail = 0;
    std::cout << "=== L1 ===\n";

    {   // get on empty DB
        DB db;
        CHECK_NONE("empty_get", db.get("k", "f"));
    }
    {   // basic set/get
        DB db;
        CHECK("set_basic", db.set("k", "f", 5));
        CHECK_EQ("set_basic", db.get("k", "f"), 5);
    }
    {   // overwrite
        DB db;
        db.set("k", "f", 1);
        db.set("k", "f", 2);
        CHECK_EQ("overwrite", db.get("k", "f"), 2);
    }
    {   // field / key isolation
        DB db;
        db.set("k1", "f1", 1);
        db.set("k1", "f2", 2);
        db.set("k2", "f1", 3);
        CHECK_EQ("isolation", db.get("k1", "f1"), 1);
        CHECK_EQ("isolation", db.get("k1", "f2"), 2);
        CHECK_EQ("isolation", db.get("k2", "f1"), 3);
        CHECK_NONE("isolation", db.get("k2", "f2"));   // key exists, field doesn't
        CHECK_NONE("isolation", db.get("k3", "f1"));   // key doesn't exist
    }
    {   // del
        DB db;
        db.set("k", "f1", 1);
        db.set("k", "f2", 2);
        CHECK("del_existing", db.del("k", "f1"));
        CHECK_NONE("del_existing", db.get("k", "f1"));
        CHECK_EQ("del_existing", db.get("k", "f2"), 2); // sibling untouched
        CHECK("del_twice", !db.del("k", "f1"));
        CHECK("del_missing_field", !db.del("k", "nope"));
        CHECK("del_missing_key", !db.del("nope", "f2"));
    }
    {   // del last field, then reuse key
        DB db;
        db.set("k", "f", 1);
        CHECK("del_last", db.del("k", "f"));
        CHECK_NONE("del_last", db.get("k", "f"));
        CHECK("reuse_key", db.set("k", "g", 7));
        CHECK_EQ("reuse_key", db.get("k", "g"), 7);
    }
    {   // edge values
        DB db;
        db.set("", "", 0);
        db.set("k", "min", INT_MIN);
        db.set("k", "max", INT_MAX);
        CHECK_EQ("empty_strings", db.get("", ""), 0);
        CHECK_EQ("int_min", db.get("k", "min"), INT_MIN);
        CHECK_EQ("int_max", db.get("k", "max"), INT_MAX);
    }
    {   // const correctness: must compile
        DB db;
        db.set("k", "f", 9);
        const DB& cdb = db;
        CHECK_EQ("const_get", cdb.get("k", "f"), 9);
    }

    std::cout << (g_fail ? "FAILED" : "PASSED")
              << "  (" << g_pass << " passed, " << g_fail << " failed)\n";
    return g_fail == 0;
}
