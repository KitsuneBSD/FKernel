#pragma once

#include <LibFK/Container/Sequence/intrusive_list.h>

struct Task;

struct TaskNodes {
  fk::containers::IntrusiveListNode<struct Task> run;
  fk::containers::IntrusiveListNode<struct Task> wait;
  fk::containers::IntrusiveListNode<struct Task> sleep;
  fk::containers::IntrusiveListNode<struct Task> zombie;
};
