/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once

namespace csql {

template <class T>
QueryTreeCoderType<T>::QueryTreeCoderType() {
  QueryTreeCoder::registerType(
      &typeid(T),
      T::kSerializableID,
      [] (RefPtr<QueryTreeNode> self, stx::OutputStream* os) {
        T::encode(*dynamic_cast<T*>(self.get()), os);
      },
      [] (stx::InputStream* is) -> RefPtr<QueryTreeNode> {
        return T::decode(is);
      });
}

} // namespace csql
