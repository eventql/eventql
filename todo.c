//    if (!qstr.isEmpty()) {
//      auto qstr_norm = normalize_(lang, qstr.get());
//      qry_obj.addChild(searchq_schema->fieldId("query_string"), qstr.get());
//      qry_obj.addChild(searchq_schema->fieldId("query_string_normalized"), qstr_norm);
//    }

//      auto docid = item.item.docID();
//
//      auto shopid = get_field_(docid, "shop_id");
//      if (shopid.isEmpty()) {
//        //stx::logWarning(
//        //    "cm.logjoin",
//        //    "item not found in featureindex: $0",
//        //    docid.docid);
//      } else {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("shop_id"),
//            (uint32_t) std::stoull(shopid.get()));
//      }
//
//      auto category1 = get_field_(docid, "category1");
//      if (!category1.isEmpty()) {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("category1"),
//            (uint32_t) std::stoull(category1.get()));
//      }
//
//      auto category2 = get_field_(docid, "category2");
//      if (!category2.isEmpty()) {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("category2"),
//            (uint32_t) std::stoull(category2.get()));
//      }
//
//      auto category3 = get_field_(docid, "category3");
//      if (!category3.isEmpty()) {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("category3"),
//            (uint32_t) std::stoull(category3.get()));
//      }
//
//      /* DAWANDA HACK */
//      if (item.position <= 4 && slrid.isEmpty()) {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("is_paid_result"),
//            msg::TRUE);
//      }
//
//      if ((item.position > 40 && slrid.isEmpty()) ||
//          StringUtil::beginsWith(query_type, "recos_")) {
//        item_obj.addChild(
//            searchq_item_schema->fieldId("is_recommendation"),
//            msg::TRUE);
//      }
//      /* EOF DAWANDA HACK */
