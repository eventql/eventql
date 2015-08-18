/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/EventIngress.h>
#include <tsdb/TimeWindowPartitioner.h>
#include <stx/json/json.h>
#include <stx/logging.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace cm {

EventIngress::EventIngress(tsdb::TSDBClient* tsdb) : tsdb_(tsdb) {}

void EventIngress::insertEvents(
    const String& customer,
    const String& event_type,
    const Buffer& data) {

  if (event_type == "ecommerce_transactions") {
    return insertECommerceTransactionsJSON(customer, data);
  }

  RAISEF(kRuntimeError, "event type not found: '$0'", event_type);
}

void EventIngress::insertECommerceTransactionsJSON(
    const String& customer,
    const Buffer& data) {
  List<Event> events;

  auto js = json::parseJSON(data);
  auto js_transactions = json::JSONUtil::objectLookup(
      js.begin(),
      js.end(),
      "transactions");

  if (js_transactions == js.end()) {
    RAISE(kIllegalArgumentError, "missing 'transactions' key");
  }

  auto cur = js_transactions;
  auto end = js_transactions + js_transactions->size;
  for (++cur; cur < end; cur += cur->size) {
    auto cend = cur + cur->size;
    if (cur->type == json::JSON_ARRAY_END) {
      break;
    }

    ECommerceTransaction ev;
    ev.set_transaction_type(ECOMMERCE_TX_PURCHASE);

    ev.set_transaction_id(
        json::JSONUtil::objectGetString(cur, cend, "transaction_id").get());

    ev.set_time(
        json::JSONUtil::objectGetUInt64(cur, cend, "time").get());

    auto items = json::JSONUtil::objectLookup(cur, cend, "items");
    auto num_items = json::JSONUtil::arrayLength(items, cend);
    for (size_t i = 0; i < num_items; ++i) {
      auto item = json::JSONUtil::arrayLookup(items, cend, i);
      auto ev_item = ev.add_items();

      ev_item->set_item_id(
          json::JSONUtil::objectGetString(item, cend, "item_id").get());

      ev_item->set_shop_id(
          json::JSONUtil::objectGetUInt64(item, cend, "shop_id").get());

      ev_item->set_price_cents(
          json::JSONUtil::objectGetUInt64(item, cend, "price_cents").get());

      auto currency_str =
          json::JSONUtil::objectGetString(item, cend, "currency").get();

      Currency currency;
      Currency_Parse(currency_str, &currency);
      ev_item->set_currency(currency);

      if (json::JSONUtil::objectGetBool(item, cend, "refunded").get()) {
        ev.set_transaction_type(ECOMMERCE_TX_REFUND);
      }
    }

    auto key = StringUtil::format(
        "$0~$1~$2",
        ev.transaction_id(),
        ECommerceTransactionType_Name(ev.transaction_type()),
        ev.time());

    events.emplace_back(
        SHA1::compute(key),
        ev.time() * kMicrosPerSecond,
        *msg::encode(ev));
  }

  insertEvents(customer, "events.ecommerce_transactions", events);
}

void EventIngress::insertEvents(
    const String& customer,
    const String& table_name,
    const List<Event>& events) {
  stx::logDebug(
      "analytics",
      "[ingress] inserting $0 events; customer=$1 stream=$2",
      events.size(),
      customer,
      table_name);

  tsdb::RecordEnvelopeList records;
  for (const auto& e : events) {
    auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
        table_name,
        e.time,
        4 * kMicrosPerHour); // FIXPAUL

    auto r = records.add_records();
    r->set_tsdb_namespace(customer);
    r->set_table_name(table_name);
    r->set_partition_sha1(partition_key.toString());
    r->set_record_id(e.id.toString());
    r->set_record_data(e.data.data(), e.data.size());
  }

  tsdb_->insertRecords(records);
}

} // namespace cm
