/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/SFunction.h>

#include "eventql/eventql.h"

namespace csql {

SFunction::SFunction(
    std::vector<SType> _arg_types,
    SType _return_type,
    void (*_call)(sql_txn* ctx, VMStack* stack),
    bool _has_side_effects /* = false */) :
    type(FN_PURE),
    arg_types(_arg_types),
    return_type(_return_type),
    has_side_effects(_has_side_effects) {
  vtable.call = _call;
}

SFunction::SFunction(
    std::vector<SType> _arg_types,
    SType _return_type,
    size_t _instance_size,
    void (*_accumulate)(sql_txn*, void* self, VMStack* stack),
    void (*_get)(sql_txn*, void* self, VMStack* stack),
    void (*_reset)(sql_txn*, void* self),
    void (*_init)(sql_txn*, void* self),
    void (*_free)(sql_txn*, void* self),
    void (*_merge)(sql_txn*, void* self, const void* other),
    void (*_savestate)(sql_txn*, void* self, OutputStream* os),
    void (*_loadstate)(sql_txn*, void* self, InputStream* is)) :
    type(FN_AGGREGATE),
    instance_size(_instance_size),
    arg_types(_arg_types),
    return_type(_return_type) {
  vtable.accumulate = _accumulate;
  vtable.get = _get;
  vtable.reset = _reset;
  vtable.init = _init;
  vtable.free = _free;
  vtable.merge = _merge;
  vtable.savestate = _savestate;
  vtable.loadstate = _loadstate;
}

} // namespace csql

