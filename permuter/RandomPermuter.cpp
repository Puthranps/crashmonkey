#include <iterator>
#include <list>
#include <numeric>
#include <vector>

#include "RandomPermuter.h"

namespace fs_testing {
namespace permuter {
using std::advance;
using std::iota;
using std::list;
using std::mt19937;
using std::uniform_int_distribution;
using std::vector;

using fs_testing::utils::disk_write;

static struct epoch_op epoch_op_null;

bool epoch_op::is_null() {
  return &epoch_op_null == this;
}

void RandomPermuter::init_data_vector(vector<disk_write> *data) {
  log_length = data->size();
  unsigned int index = 0;
  while (index < data->size()) {
    struct epoch current_epoch;
    // Epochs don't have to start with a sync op.
    int epoch_sync_index = -1;

    // Get all ops in this epoch and add them to either the sync_op or async_op
    // lists.
    while (index < data->size() && !(data->at(index)).is_barrier_write()) {
      ++current_epoch.length;
      struct epoch_op info;
      info.write = data->at(index);
      // Since we start with an invalid index we need to pre-increment for
      // actual sync operations.
      if (!info.write.is_async_write()) {
        ++epoch_sync_index;
      }
      info.nearest_sync = epoch_sync_index;

      if (info.write.is_async_write()) {
        current_epoch.async_ops.push_back(info);
      } else {
        current_epoch.sync_ops.push_back(info);
      }

      ++index;
    }

    // Check is the op at the current index is a "barrier." If it is then add it
    // to the special spot in the epoch, otherwise just push the current epoch
    // onto the list and move to the next segment of the log.
    if ((data->at(index)).is_barrier_write()) {
      ++current_epoch.length;
      current_epoch.barrier_op.write = data->at(index);
      current_epoch.barrier_op.nearest_sync = epoch_sync_index;
      ++index;
    }
    epochs.push_back(current_epoch);
  }
}

RandomPermuter::RandomPermuter() {
  log_length = 0;
  rand = mt19937(42);
}

RandomPermuter::RandomPermuter(vector<disk_write> *data) {
  init_data_vector(data);
  // TODO(ashmrtn): Make a flag to make it random or not.
  rand = mt19937(42);
}

void RandomPermuter::set_data(vector<disk_write> *data) {
  init_data_vector(data);
}

bool RandomPermuter::permute(vector<disk_write> *res) {
  res->clear();
  res = new vector<disk_write>(log_length);
  int current_index = 0;
  for (struct epoch current_epoch : epochs) {
    current_index = permute_epoch(res, current_index, current_epoch);
  }

  return true;
}

int RandomPermuter::permute_epoch(vector<disk_write> *res,
    const int start_index, epoch& epoch) {

  unsigned int slots = epoch.length;

  // Place the barrier operation if it exists since the entire vector already
  // exists (i.e. we won't cause extra shifting when adding the other elements).
  // Decrement out count of empty slots since we have filled one.
  if (!epoch.barrier_op.is_null()) {
    res->at(start_index + slots - 1) = epoch.barrier_op.write;
    --slots;
  }

  // Fill the list with the empty slots, either [0, epoch.length - 1) or
  // [0, epoch.length - 2).
  list<unsigned int> empty_slots(slots);
  iota(empty_slots.begin(), empty_slots.end(), 0);

  // TODO(ashmrtn): Fixme.
  unsigned int current_sync_index = -1;
  unsigned int current_index = start_index;
  for (struct epoch_op op : epoch.async_ops) {
    // If the closest sync operation for this async_op is greater than our
    // current_sync_index we need to add more sync ops so that we maintain an
    // ordering.
    while (op.nearest_sync > current_sync_index) {
      res->at(current_index++) = epoch.sync_ops.at(++current_sync_index).write;
    }
    uniform_int_distribution<unsigned int> uid(0, empty_slots.size());
    auto shift = empty_slots.begin();
    advance(shift, uid(rand));
    res->at(current_index + *shift) = op.write;
    ++current_index;
    empty_slots.erase(shift);
  }

  // Finish off by placing the rest of the sync ops in order in the result.
  // Increment current_sync_index by one because until now it has been tracking
  // the last sync_op we HAVE placed. Now we want to place any remaining
  // sync_ops which would be sync_ops we HAVEN'T YET placed.
  ++current_sync_index;
  while (current_sync_index < epoch.sync_ops.size()) {
    res->at(current_index++) = epoch.sync_ops.at(++current_sync_index).write;
  }

  return start_index + epoch.length;
}

}  // namespace permuter
}  // namespace fs_testing

extern "C" fs_testing::permuter::Permuter* permuter_get_instance(
    std::vector<fs_testing::utils::disk_write> *data) {
  return new fs_testing::permuter::RandomPermuter(data);
}

extern "C" void permuter_delete_instance(fs_testing::permuter::Permuter* p) {
  delete p;
}