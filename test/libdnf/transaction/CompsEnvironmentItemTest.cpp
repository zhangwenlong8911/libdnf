#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

#include "libdnf/transaction/CompsEnvironmentItem.hpp"
#include "libdnf/transaction/Transformer.hpp"

#include "CompsEnvironmentItemTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(CompsEnvironmentItemTest);

using namespace libdnf::transaction;

void
CompsEnvironmentItemTest::setUp()
{
    conn = new libdnf::utils::SQLite3(":memory:");
    Transformer::createDatabase(*conn);
}

void
CompsEnvironmentItemTest::tearDown()
{
    delete conn;
}

static std::shared_ptr< CompsEnvironmentItem >
createCompsEnvironment(Transaction & trans)
{
    auto env = std::make_shared< CompsEnvironmentItem >(trans);
    env->setEnvironmentId("minimal");
    env->setName("Minimal Environment");
    env->setTranslatedName("translated(Minimal Environment)");
    env->setPackageTypes(CompsPackageType::DEFAULT);
    env->addGroup("core", true, CompsPackageType::MANDATORY);
    env->addGroup("base", false, CompsPackageType::OPTIONAL);
    env->save();
    return env;
}

void
CompsEnvironmentItemTest::testCreate()
{
    Transaction trans(*conn);
    auto env = createCompsEnvironment(trans);

    CompsEnvironmentItem env2(trans, env->getId());
    CPPUNIT_ASSERT(env2.getId() == env->getId());
    CPPUNIT_ASSERT(env2.getEnvironmentId() == env->getEnvironmentId());
    CPPUNIT_ASSERT(env2.getName() == env->getName());
    CPPUNIT_ASSERT(env2.getTranslatedName() == env->getTranslatedName());
    CPPUNIT_ASSERT(env2.getPackageTypes() == env->getPackageTypes());

    {
        auto group = env2.getGroups().at(0);
        CPPUNIT_ASSERT(group->getGroupId() == "base");
        CPPUNIT_ASSERT(group->getInstalled() == false);
        CPPUNIT_ASSERT(group->getGroupType() == CompsPackageType::OPTIONAL);
    }
    {
        auto group = env2.getGroups().at(1);
        CPPUNIT_ASSERT(group->getGroupId() == "core");
        CPPUNIT_ASSERT(group->getInstalled() == true);
        CPPUNIT_ASSERT(group->getGroupType() == CompsPackageType::MANDATORY);
    }

    // test adding a duplicate group
    env2.addGroup("base", true, CompsPackageType::MANDATORY);
    {
        auto group = env2.getGroups().at(0);
        CPPUNIT_ASSERT(group->getGroupId() == "base");
        CPPUNIT_ASSERT(group->getInstalled() == true);
        CPPUNIT_ASSERT(group->getGroupType() == CompsPackageType::MANDATORY);
    }
}

void
CompsEnvironmentItemTest::testGetTransactionItems()
{
    Transaction trans(*conn);
    auto env = createCompsEnvironment(trans);
    auto ti = trans.addItem(env, "", TransactionItemAction::INSTALL, TransactionItemReason::USER);
    ti->set_state(TransactionItemState::DONE);
    trans.begin();
    trans.finish(TransactionState::DONE);

    Transaction trans2(*conn, trans.get_id());

    auto transItems = trans2.getItems();
    CPPUNIT_ASSERT_EQUAL(1, static_cast< int >(transItems.size()));

    auto transItem = transItems.at(0);

    auto env2 = std::dynamic_pointer_cast<CompsEnvironmentItem>(transItem->getItem());
    {
        auto group = env2->getGroups().at(0);
        CPPUNIT_ASSERT_EQUAL(std::string("base"), group->getGroupId());
    }
    {
        auto group = env2->getGroups().at(1);
        CPPUNIT_ASSERT_EQUAL(std::string("core"), group->getGroupId());
    }
}
