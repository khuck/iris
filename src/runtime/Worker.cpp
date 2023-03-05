#include "Worker.h"
#include "Debug.h"
#include "Consistency.h"
#include "Device.h"
#include "Platform.h"
#include "QueueReady.h"
#include "Scheduler.h"
#include "Task.h"
#include "Timer.h"

namespace iris {
namespace rt {

Worker::Worker(Device* dev, Platform* platform, bool single) {
  dev_ = dev;
  platform_ = platform;
  dev_->set_worker(this);
  scheduler_ = platform_->scheduler();
  if (scheduler_) consistency_ = scheduler_->consistency();
  else consistency_ = NULL;
  single_ = single;
  if (single) queue_ = platform->queue();
  else queue_ = new QueueReady();
  busy_ = false;
}

Worker::~Worker() {
  _trace("Worker is destroyed\n");
  if (!single_) delete queue_;
}

void Worker::TaskComplete(Task* task) {
  _trace("now invoke scheduler after task:%lu:%s qsize:%lu\n", task->uid(), task->name(), queue_->Size());
  if (scheduler_) scheduler_->Invoke();
  _trace("now invoke worker after task:%lu:%s\n qsize:%lu\n", task->uid(), task->name(), queue_->Size());
  Invoke();
}

void Worker::Enqueue(Task* task) {
  //if we're running an internal task (such as an internal memory movement for consistency) skip the queue.
  if (task->is_internal_memory_transfer()) {
    dev_->Synchronize();
    Execute(task);
    return;
  }
  _trace("Enqueuing task:%lu:%s\n", task->uid(), task->name());
  while (!queue_->Enqueue(task)) { }
  _trace("Invoking worker for task:%lu:%s qsize:%lu\n", task->uid(), task->name(), queue_->Size());
  Invoke();
}

void Worker::Execute(Task* task) {
  if (!task->Executable()) return;
  task->set_dev(dev_);
  if (task->marker()) {
    dev_->Synchronize();
    task->Complete();
    //task->TryReleaseTask();
    return;
  }
  busy_ = true;
  if (scheduler_) scheduler_->StartTask(task, this);
  if (consistency_) consistency_->Resolve(task);
  bool task_cmd_last = task->cmd_last();
  dev_->Execute(task);
  if (!task_cmd_last) {
    if (scheduler_) scheduler_->CompleteTask(task, this);
    //task->Complete();
  }
  //task->TryReleaseTask();
  busy_ = false;
}

void Worker::Run() {
  while (true) {
    _trace("Worker entering into sleep mode\n");
    Sleep();
    _trace("Came out from sleep Device:%d:%s Queue size:%lu", dev_->devno(), dev_->name(), queue_->Size());
    if (!running_) break;
    Task* task = NULL;
    _trace("Device:%d:%s Queue size:%lu", dev_->devno(), dev_->name(), queue_->Size());
    while (running_ && queue_->Dequeue(&task)){
      _trace("Device:%d:%s Qsize:%lu dequeued task:%lu:%s", dev_->devno(), dev_->name(), queue_->Size(), task->uid(), task->name());
      Execute(task);
      _trace("Completed task Device:%d:%s Qsize:%lu dequeued task:%lu:%s", dev_->devno(), dev_->name(), queue_->Size(), task->uid(), task->name());
    }
  }
}

unsigned long Worker::ntasks() {
  return queue_->Size() + (busy_ ? 1 : 0);
}

} /* namespace rt */
} /* namespace iris */
