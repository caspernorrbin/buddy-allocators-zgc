#ifndef BUDDY_HELPER_HPP
#define BUDDY_HELPER_HPP
#include <cstddef>

struct double_link {
  double_link *prev;
  double_link *next;
};

class BuddyHelper {
public:
  static inline bool list_empty(double_link *head) { return head->next == head; }

  static void list_remove(double_link *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node;
    node->next = node;
  }

  static void push_back(double_link *head, double_link *node) {
    node->prev = head->prev;
    node->next = head;
    head->prev->next = node;
    head->prev = node;
  }

  static inline double_link *pop_first(double_link *head) {
    if (list_empty(head)) {
      return nullptr;
    }

    double_link *first = head->next;
    first->next->prev = head;
    head->next = first->next;

    // first->prev = first;
    // first->next = first;
    return first;
  }

  static bool bit_is_set(const unsigned char *bitmap, int index) {
    return static_cast<bool>(bitmap[index / 8] &
                             (1U << (static_cast<unsigned int>(index) % 8)));
  }

  static void set_bit(unsigned char *bitmap, int index) {
    bitmap[index / 8] |= (1U << (static_cast<unsigned int>(index) % 8));
  }

  static void clear_bit(unsigned char *bitmap, int index) {
    bitmap[index / 8] &= ~(1U << (static_cast<unsigned int>(index) % 8));
  }

  static void flip_bit(unsigned char *bitmap, int index) {
    bitmap[index / 8] ^= (1U << (static_cast<unsigned int>(index) % 8));
  }

  static size_t round_up_pow2(size_t size) {
    if (size == 0) {
      return 1;
    }

    size--;
    size |= size >> 1U;
    size |= size >> 2U;
    size |= size >> 4U;
    size |= size >> 8U;
    size |= size >> 16U;
    size++;

    return size;
  }
};

#endif // BUDDY_HELPER_HPP
