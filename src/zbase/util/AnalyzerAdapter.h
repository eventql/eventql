/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_ANALZERADAPTER_H
#define _FNORD_FTS_ANALZERADAPTER_H

#include "stx/autoref.h"
#include "fnord-fts/Analyzer.h"
#include "fnord-fts/analysis/TokenStream.h"
#include "fnord-fts/util/CloseableThreadLocal.h"

namespace stx {
namespace fts {

class AnalyzerAdapter : public LuceneObject {
public:
  AnalyzerAdapter(RefPtr<Analyzer> analyzer);
  ~AnalyzerAdapter();
  LUCENE_CLASS(AnalyzerAdapter);

  TokenStreamPtr tokenStream(const String& fieldName, const ReaderPtr& reader);
  TokenStreamPtr reusableTokenStream(const String& fieldName, const ReaderPtr& reader);
  int32_t getPositionIncrementGap(const String& fieldName);
  int32_t getOffsetGap(const FieldablePtr& field);
  void close();

protected:
  RefPtr<Analyzer> analyzer_;
  CloseableThreadLocal<LuceneObject> tokenStreams;
  LuceneObjectPtr getPreviousTokenStream();
  void setPreviousTokenStream(const LuceneObjectPtr& stream);
};

}
}
#endif
