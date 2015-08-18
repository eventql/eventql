/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/logging.h>
#include "zbase/TermInfo.h"
#include "zbase/TermInfoTableSource.h"
#include "AutoCompleteModel.h"

using namespace stx;

namespace cm {

RefPtr<AutoCompleteModel> AutoCompleteModel::fromCache(
    const String& customer,
    ModelCache* cache) {
  return dynamic_cast<AutoCompleteModel*>(cache->getModel(
      "AutoCompleteModel",
      "termstats",
      "termstats-" + customer));
}

AutoCompleteModel::AutoCompleteModel(
    const String& filename,
    RefPtr<fts::Analyzer> analyzer) : 
    analyzer_(analyzer) {
  cat_names_.emplace("de~e1-1",  "Mode");
  cat_names_.emplace("de~e1-2",  "Accessoires");
  cat_names_.emplace("de~e1-3",  "Taschen");
  cat_names_.emplace("de~e1-4",  "Schmuck");
  cat_names_.emplace("de~e1-5",  "Kinder");
  cat_names_.emplace("de~e1-6",  "Wohnen");
  cat_names_.emplace("de~e1-7",  "Kunst");
  cat_names_.emplace("de~e1-8",  "Handarbeitsbedarf");
  cat_names_.emplace("de~e1-9",  "Vintage");
  cat_names_.emplace("de~e1-10", "Baby");
  cat_names_.emplace("de~e1-11", "Anlass");
  cat_names_.emplace("de~e1-12", "Sale");
  cat_names_.emplace("de~e1-13", "Maenner");
  cat_names_.emplace("de~e1-14", "Schreibwaren");
  cat_names_.emplace("en~e1-1",  "Fashion");
  cat_names_.emplace("en~e1-2",  "Accessoires");
  cat_names_.emplace("en~e1-3",  "Bags");
  cat_names_.emplace("en~e1-4",  "Jewellery");
  cat_names_.emplace("en~e1-5",  "Children");
  cat_names_.emplace("en~e1-6",  "Home");
  cat_names_.emplace("en~e1-7",  "Art");
  cat_names_.emplace("en~e1-8",  "Supplies");
  cat_names_.emplace("en~e1-9",  "Vintage");
  cat_names_.emplace("en~e1-10", "Baby");
  cat_names_.emplace("en~e1-11", "Occasions");
  cat_names_.emplace("en~e1-12", "Sale");
  cat_names_.emplace("en~e1-13", "Men");
  cat_names_.emplace("en~e1-14", "Stationery");
  cat_names_.emplace("fr~e1-1",  "Mode");
  cat_names_.emplace("fr~e1-2",  "Accessoires");
  cat_names_.emplace("fr~e1-3",  "Sacs");
  cat_names_.emplace("fr~e1-4",  "Bijoux");
  cat_names_.emplace("fr~e1-5",  "Enfants");
  cat_names_.emplace("fr~e1-6",  "Deco");
  cat_names_.emplace("fr~e1-7",  "Art");
  cat_names_.emplace("fr~e1-8",  "Materiel");
  cat_names_.emplace("fr~e1-9",  "Vintage");
  cat_names_.emplace("fr~e1-10", "Bebes");
  cat_names_.emplace("fr~e1-11", "Occasion");
  cat_names_.emplace("fr~e1-12", "Soldes");
  cat_names_.emplace("fr~e1-13", "Hommes");
  cat_names_.emplace("fr~e1-14", "Papeterie");
  cat_names_.emplace("es~e1-1",  "Moda");
  cat_names_.emplace("es~e1-2",  "Accesorios");
  cat_names_.emplace("es~e1-3",  "Bolsos");
  cat_names_.emplace("es~e1-4",  "Joyas y Bisutería");
  cat_names_.emplace("es~e1-5",  "Niños");
  cat_names_.emplace("es~e1-6",  "Hogar");
  cat_names_.emplace("es~e1-7",  "Arte");
  cat_names_.emplace("es~e1-8",  "Material");
  cat_names_.emplace("es~e1-9",  "Vintage");
  cat_names_.emplace("es~e1-10", "Bebés");
  cat_names_.emplace("es~e1-11", "Oportunidades");
  cat_names_.emplace("es~e1-12", "REBAJAS");
  cat_names_.emplace("es~e1-13", "Hombre");
  cat_names_.emplace("es~e1-14", "Papelería");
  cat_names_.emplace("nl~e1-1",  "Mode");
  cat_names_.emplace("nl~e1-2",  "Accessoires");
  cat_names_.emplace("nl~e1-3",  "Tassen");
  cat_names_.emplace("nl~e1-4",  "Sieraden");
  cat_names_.emplace("nl~e1-5",  "Kinderen");
  cat_names_.emplace("nl~e1-6",  "Wonen");
  cat_names_.emplace("nl~e1-7",  "Kunst");
  cat_names_.emplace("nl~e1-8",  "Materialen");
  cat_names_.emplace("nl~e1-9",  "Vintage");
  cat_names_.emplace("nl~e1-10", "Baby´s");
  cat_names_.emplace("nl~e1-11", "Evenementen");
  cat_names_.emplace("nl~e1-12", "Sale");
  cat_names_.emplace("nl~e1-13", "Mannen");
  cat_names_.emplace("nl~e1-14", "Schrijfwaren");
  cat_names_.emplace("pl~e1-1",  "Moda");
  cat_names_.emplace("pl~e1-2",  "Akcesoria");
  cat_names_.emplace("pl~e1-3",  "Torebki");
  cat_names_.emplace("pl~e1-4",  "Biżuteria");
  cat_names_.emplace("pl~e1-5",  "Dzieci");
  cat_names_.emplace("pl~e1-6",  "Dom i styl życia");
  cat_names_.emplace("pl~e1-7",  "Sztuka");
  cat_names_.emplace("pl~e1-8",  "Materiały");
  cat_names_.emplace("pl~e1-9",  "Vintage");
  cat_names_.emplace("pl~e1-10", "Niemowlaki");
  cat_names_.emplace("pl~e1-11", "Specjalne okazje");
  cat_names_.emplace("pl~e1-12", "Promocje");
  cat_names_.emplace("pl~e1-13", "Mężczyźni");
  cat_names_.emplace("pl~e1-14", "Artykuły papiernicze");
  cat_names_.emplace("it~e1-1",  "Abbagliamento");
  cat_names_.emplace("it~e1-2",  "Accessori");
  cat_names_.emplace("it~e1-3",  "Borse");
  cat_names_.emplace("it~e1-4",  "Gioielli");
  cat_names_.emplace("it~e1-5",  "Bambini");
  cat_names_.emplace("it~e1-6",  "Casa");
  cat_names_.emplace("it~e1-7",  "Arte");
  cat_names_.emplace("it~e1-8",  "Materiali");
  cat_names_.emplace("it~e1-9",  "Vintage");
  cat_names_.emplace("it~e1-10", "Neonati");
  cat_names_.emplace("it~e1-11", "Eventi");
  cat_names_.emplace("it~e1-12", "OFFERTE");
  cat_names_.emplace("it~e1-13", "Uomo");
  cat_names_.emplace("it~e1-14", "Cartoleria");

  loadModelFile(filename);
}

AutoCompleteModel::~AutoCompleteModel() {
  stx::logDebug("cm.autocompletemodel", "Free'ing model <$0>");
}

void AutoCompleteModel::loadModelFile(const String& filename) {
  stx::logDebug(
      "cm.autocompletemodel",
      "Loading model <$0> from: $1",
      filename);

  TermInfoTableSource tbl(Set<String> { filename });
  tbl.forEach(std::bind(
      &AutoCompleteModel::addTermInfo,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
  tbl.read();
}

void AutoCompleteModel::addTermInfo(const String& term, const TermInfo& ti) {
  if (ti.score < 1000) return;
  term_info_.emplace(term, ti);
}

void AutoCompleteModel::suggest(
    Language lang,
    const String& qstr,
    ResultListType* results) {
  auto lang_str = languageToString(lang);

  Vector<String> terms;
  Vector<String> valid_terms;
  analyzer_->tokenize(lang, qstr, &terms);
  if (terms.size() == 0) {
    return;
  }

  for (const auto& t : terms) {
    if (term_info_.count(lang_str + "~" + t) > 0) {
      valid_terms.emplace_back(t);
    }
  }

  if (terms.size() == 1 || valid_terms.size() == 0) {
    suggestSingleTerm(lang, terms, results);
  } else {
    suggestMultiTerm(lang, terms, valid_terms, results);
  }

#ifndef FNORD_NODEBUG
  stx::logDebug(
      "cm.autocompletemodel",
      "suggest lang=$0 qstr=$1 terms=$2 valid_terms=$3 results=$4",
      lang_str,
      qstr,
      inspect(terms),
      inspect(valid_terms),
      results->size());
#endif
}

void AutoCompleteModel::suggestSingleTerm(
    Language lang,
    Vector<String> terms,
    ResultListType* results) {
  auto lang_str = stx::languageToString(lang);
  auto prefix = lang_str + "~" + terms.back();
  terms.pop_back();
  String qstr_prefix;

  if (terms.size() > 0 ) {
    qstr_prefix += StringUtil::join(terms, " ") + " ";
  }

  Vector<Pair<String, double>> matches;
  double best_match = 0;

  for (auto iter = term_info_.lower_bound(prefix);
      iter != term_info_.end() && StringUtil::beginsWith(iter->first, prefix);
      ++iter) {
    matches.emplace_back(iter->first, iter->second.score);
    if (iter->second.score > best_match) {
      best_match = iter->second.score;
    }
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  if (matches.size() > 0) {
    const auto& best_match_ti = term_info_.find(matches[0].first);
    for (const auto& c : best_match_ti->second.top_categories) {
      if ((c.second / best_match_ti->second.top_categories[0].second) < 0.2) {
        break;
      }

      auto qstr = qstr_prefix + matches[0].first.substr(3);
      auto label = StringUtil::format(
          "$0 in $1",
          qstr,
          cat_names_[lang_str + "~" + c.first]);

      AutoCompleteResult res;
      res.text = label;
      res.score = c.second;
      res.attrs.emplace("query_string", qstr);
      res.attrs.emplace("category_id", c.first);
      results->emplace_back(res);
    }
  }

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    AutoCompleteResult res;
    res.text = qstr_prefix + matches[m].first.substr(3);
    res.score = score;
    res.attrs.emplace("query_string", res.text);
    results->emplace_back(res);
  }

  if (matches.size() > 0) {
    const auto& best_match_ti = term_info_.find(matches[0].first);

    for (const auto& r : best_match_ti->second.related_terms) {
      auto label = StringUtil::format(
          "$0$1 $2",
          qstr_prefix,
          matches[0].first.substr(3),
          r.first);

      AutoCompleteResult res;
      res.text = label;
      res.score = r.second;
      res.attrs.emplace("query_string", label);
      results->emplace_back(res);
    }
  }
}

void AutoCompleteModel::suggestMultiTerm(
    Language lang,
    Vector<String> terms,
    const Vector<String>& valid_terms,
    ResultListType* results) {
  Set<String> search_terms;
  auto lang_str = stx::languageToString(lang);
  auto last_term = terms.back();
  terms.pop_back();
  String qstr_prefix;

  if (terms.size() > 0 ) {
    qstr_prefix += StringUtil::join(terms, " ") + " ";
  }

  HashMap<String, double> topcats_h;
  double best_topcat = 0;
  HashMap<String, double> matches_h;
  double best_match = 0;
  for (const auto& vt : valid_terms) {
    const auto& vtinfo = term_info_.find(lang_str + "~" + vt)->second;
    search_terms.emplace(vt);

    for (const auto& topcat : vtinfo.top_categories) {
      topcats_h[topcat.first] += topcat.second;
      topcats_h[topcat.first] *= 2;

      if (topcats_h[topcat.first] > best_topcat) {
        best_topcat = topcats_h[topcat.first];
      }
    }

    for (const auto& related : vtinfo.related_terms) {
      if (!StringUtil::beginsWith(related.first, last_term)) {
        continue;
      }

      matches_h[related.first] += related.second;

      if (matches_h[related.first] > best_match) {
        best_match = matches_h[related.first];
      }
    }
  }

  Vector<Pair<String, double>> topcats;
  for (const auto& m : topcats_h) {
    topcats.emplace_back(m);
  }

  std::sort(topcats.begin(), topcats.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  for (int m = 0; m < topcats.size() && m < 3; ++m) {
    auto score = topcats[m].second;

    if ((score / best_topcat) < 0.2) {
      break;
    }

    auto qstr = StringUtil::join(valid_terms, " ");
    auto label = StringUtil::format(
        "$0 in $1",
        qstr,
        cat_names_[lang_str + "~" + topcats[m].first]);

    AutoCompleteResult res;
    res.text = label;
    res.score = score;
    res.attrs.emplace("query_string", qstr);
    res.attrs.emplace("category_id", topcats[m].first);
    results->emplace_back(res);
  }

  Vector<Pair<String, double>> matches;
  for (const auto& m : matches_h) {
    matches.emplace_back(m);
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  if (matches.size() > 0) {
    search_terms.emplace(matches[0].first);
  }

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    AutoCompleteResult res;
    res.text = qstr_prefix + matches[m].first;
    res.score = score;
    res.attrs.emplace("query_string", res.text);
    results->emplace_back(res);
  }

  matches_h.clear();
  matches.clear();
  best_match = 0;

  for (const auto& vt : search_terms) {
    auto vtinfo_iter = term_info_.find(lang_str + "~" + vt);
    if (vtinfo_iter == term_info_.end()) {
      continue;
    }

    const auto& vtinfo = vtinfo_iter->second;
    for (const auto& related : vtinfo.related_terms) {
      matches_h[related.first] += related.second;

      if (matches_h[related.first] > best_match) {
        best_match = matches_h[related.first];
      }
    }
  }

  for (const auto& m : matches_h) {
    matches.emplace_back(m);
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    AutoCompleteResult res;
    res.text = qstr_prefix + matches[m].first;
    res.score = score;
    res.attrs.emplace("query_string", res.text);
    results->emplace_back(res);
  }
}

}
