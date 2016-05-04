/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_REFLECT_INDEXSEQUENCE_H
#define _STX_REFLECT_INDEXSEQUENCE_H

namespace stx {
namespace reflect {

template <int...>
struct IndexSequence {};

template <int N, int... I>
struct IndexSequenceFor :
    IndexSequenceFor<N - 1, N - 1, I...> {};

template <int... I>
struct IndexSequenceFor<0, I...>{
  typedef IndexSequence<I...> IndexSequenceType;
};

template <typename... T>
struct MkIndexSequenceFor {
  typedef typename IndexSequenceFor<sizeof...(T)>::IndexSequenceType type;
};

}
}

#endif
