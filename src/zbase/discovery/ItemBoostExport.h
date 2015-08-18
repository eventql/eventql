/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMBOOSTEXPORT_H
#define _CM_ITEMBOOSTEXPORT_H
#include "zbase/Report.h"
#include "zbase/ProtoSSTableSource.h"
#include "zbase/CSVSink.h"
#include "zbase/ProtoCRDT.h"
#include "zbase/ItemBoost.pb.h"


using namespace stx;

namespace cm {

class ItemBoostExport : public ReportRDD {
public:

  ItemBoostExport(
      RefPtr<ProtoSSTableSource<ItemBoostRow>> input,
      RefPtr<CSVSink> output);

  void onInit();
  void onFinish();

protected:
  void onRow(const String& key, const ItemBoostRow& row);

  RefPtr<ProtoSSTableSource<ItemBoostRow>> input_;
  RefPtr<CSVSink> output_;
  Vector<Pair<String, ItemBoostRow>> rows_;
};

} // namespace cm

#endif
