/**
 * Copyright (c) 2015 - The zScale Authors <legal@zscale.io>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/stdtypes.h"
#include "eventql/util/test/unittest.h"
#include "eventql/core/ReplicationScheme.h"

using namespace util;
using namespace eventql;

UNIT_TEST(ReplicationSchemeTest);

TEST_CASE(ReplicationSchemeTest, TestHostAssignmentWithoutNodes, [] () {
  ClusterConfig cc;
  cc.set_dht_num_copies(3);

  DHTReplicationScheme rscheme(cc);

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("1111111111111111111111111111111111111111"));

    EXPECT_EQ(replicas.size(), 0);
  }
});

TEST_CASE(ReplicationSchemeTest, TestHostAssignmentWithOneNode, [] () {
  ClusterConfig cc;
  cc.set_dht_num_copies(3);

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeA");
    node->set_addr("1.1.1.1:1234");
    *node->add_sha1_tokens() = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  }

  DHTReplicationScheme rscheme(cc);

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("1111111111111111111111111111111111111111"));

    EXPECT_EQ(replicas.size(), 1);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("8888888888888888888888888888888888888888"));

    EXPECT_EQ(replicas.size(), 1);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("dddddddddddddddddddddddddddddddddddddddd"));

    EXPECT_EQ(replicas.size(), 1);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
  }
});

TEST_CASE(ReplicationSchemeTest, TestHostAssignmentWithTwoNodes, [] () {
  ClusterConfig cc;
  cc.set_dht_num_copies(3);

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeA");
    node->set_addr("1.1.1.1:1234");
    *node->add_sha1_tokens() = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    *node->add_sha1_tokens() = "6666666666666666666666666666666666666666";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeB");
    node->set_addr("2.2.2.2:1234");
    *node->add_sha1_tokens() = "3333333333333333333333333333333333333333";
  }


  DHTReplicationScheme rscheme(cc);

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("1111111111111111111111111111111111111111"));

    EXPECT_EQ(replicas.size(), 2);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "1.1.1.1:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("8888888888888888888888888888888888888888"));

    EXPECT_EQ(replicas.size(), 2);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "2.2.2.2:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("dddddddddddddddddddddddddddddddddddddddd"));

    EXPECT_EQ(replicas.size(), 2);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "1.1.1.1:1234");
  }
});

TEST_CASE(ReplicationSchemeTest, TestHostAssignmentWithThreeNodes, [] () {
  ClusterConfig cc;
  cc.set_dht_num_copies(3);

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeA");
    node->set_addr("1.1.1.1:1234");
    *node->add_sha1_tokens() = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    *node->add_sha1_tokens() = "6666666666666666666666666666666666666666";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeB");
    node->set_addr("2.2.2.2:1234");
    *node->add_sha1_tokens() = "3333333333333333333333333333333333333333";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeC");
    node->set_addr("3.3.3.3:1234");
    *node->add_sha1_tokens() = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    *node->add_sha1_tokens() = "fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfc";
  }


  DHTReplicationScheme rscheme(cc);

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("1111111111111111111111111111111111111111"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "3.3.3.3:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("8888888888888888888888888888888888888888"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "3.3.3.3:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "2.2.2.2:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("dddddddddddddddddddddddddddddddddddddddd"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfc");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "3.3.3.3:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "1.1.1.1:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("fdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfd"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "3.3.3.3:1234");
  }
});

TEST_CASE(ReplicationSchemeTest, TestHostAssignmentWithFourNodes, [] () {
  ClusterConfig cc;
  cc.set_dht_num_copies(3);

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeA");
    node->set_addr("1.1.1.1:1234");
    *node->add_sha1_tokens() = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    *node->add_sha1_tokens() = "6666666666666666666666666666666666666666";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeB");
    node->set_addr("2.2.2.2:1234");
    *node->add_sha1_tokens() = "3333333333333333333333333333333333333333";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeC");
    node->set_addr("3.3.3.3:1234");
    *node->add_sha1_tokens() = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    *node->add_sha1_tokens() = "fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfc";
  }

  {
    auto node = cc.add_dht_nodes();
    node->set_status(DHTNODE_LIVE);
    node->set_name("nodeD");
    node->set_addr("4.4.4.4:1234");
    *node->add_sha1_tokens() = "1111111111111111111111111111111111111111";
    *node->add_sha1_tokens() = "7777777777777777777777777777777777777777";
  }


  DHTReplicationScheme rscheme(cc);

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("1111111111111111111111111111111111111111"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "1111111111111111111111111111111111111111");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "4.4.4.4:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "1.1.1.1:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("8888888888888888888888888888888888888888"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "3.3.3.3:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "1111111111111111111111111111111111111111");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "4.4.4.4:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("dddddddddddddddddddddddddddddddddddddddd"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfc");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "3.3.3.3:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "1111111111111111111111111111111111111111");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "4.4.4.4:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "2.2.2.2:1234");
  }

  {
    auto replicas = rscheme.replicasFor(
        SHA1Hash::fromHexString("2222222222222222222222222222222222222222"));

    EXPECT_EQ(replicas.size(), 3);
    EXPECT_EQ(
        replicas[0].unique_id.toString(),
        "3333333333333333333333333333333333333333");
    EXPECT_EQ(replicas[0].addr.ipAndPort(), "2.2.2.2:1234");
    EXPECT_EQ(
        replicas[1].unique_id.toString(),
        "6666666666666666666666666666666666666666");
    EXPECT_EQ(replicas[1].addr.ipAndPort(), "1.1.1.1:1234");
    EXPECT_EQ(
        replicas[2].unique_id.toString(),
        "7777777777777777777777777777777777777777");
    EXPECT_EQ(replicas[2].addr.ipAndPort(), "4.4.4.4:1234");
  }
});


