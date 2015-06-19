#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNode.h>


class Node : public BaseNode {
public:
  Node() : BaseNode() {}
  uint32_t test() { return 128; }
};


#endif  // #ifndef ___NODE__H___
