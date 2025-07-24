#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_PARTIAL_SUM_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_PARTIAL_SUM_H_
#include <future>
#include <iterator>
#include <numeric>
#include <thread>
#include <vector>

#include "threads_guard.hpp"

namespace playground::parallel {
template <typename it>
void PartialSum(it first, it last) {
  using ValueType = it::value_type;
  struct ChunkProcess {
    void operator()(it begin, it last, std::future<ValueType>* pre_res,
                    std::promise<ValueType>* next_res) {
      try {
        it end = last;
        ++end;
        std::partial_sum(begin, end, begin);
        if (pre_res) {
          ValueType pre_chunck_last = pre_res->get();
          *last += pre_chunck_last;
          if (next_res) {
            next_res->set_value(*last);
          }

          std::for_each(begin, last, [&pre_chunck_last](ValueType& val) {
            val += pre_chunck_last;
          });
        } else if (next_res) {
          next_res->set_value(*last);
        }
      } catch (...) {
        if (next_res) {
          next_res->set_exception(std::current_exception());
        } else {
          // 1) �ѵ�ǰ�쳣������
          std::exception_ptr eptr = std::current_exception();

          // 2) ��ָ��Ϊ�գ�˵�� catch(...) ���Լ� throw ��
          if (!eptr) {
            std::cerr << "catch(...): but no active exception?!\n";
            return;
          }

          // 3) ��ʼ�������ס�������ץ��
          try {
            std::rethrow_exception(eptr);        // ��ԭ�쳣����һ��
          } catch (const std::logic_error& e) {  // ��ץ�������/������
            std::cerr << "logic_error: " << e.what() << '\n';
          } catch (const std::runtime_error& e) {  // �ξ���
            std::cerr << "runtime_error: " << e.what() << '\n';
          } catch (const std::exception& e) {  // ���� std::exception ����
            std::cerr << "std::exception: " << e.what() << '\n';
          } catch (...) {
            // �ߵ���˵���׵��ǡ��� std::exception ����û�г����Զ������͡�
            // std::cerr << "unknown exception, typeid = "
            //<< typeid(*std::rethrow_exception(eptr)).name() << '\n';
            throw;
          }
        }
      }
    }
  };

  const unsigned long length = std::distance(first, last);
  if (!length) {
    return;
  }

  const unsigned long min_per_thread = 25;
  const unsigned long max_threads =
      (length + min_per_thread - 1) / min_per_thread;

  const unsigned long hardware_thread = std::thread::hardware_concurrency();

  const unsigned long num_threads =
      std::min(max_threads, std::max(2ul, hardware_thread));
  const unsigned long block_size = length / num_threads;

  std::vector<std::promise<ValueType>> prom_arr(num_threads - 1);
  std::vector<std::future<ValueType>> fut_arr;
  fut_arr.reserve(num_threads - 1);
  std::vector<std::thread> threads(num_threads - 1);
  ThreadsGuard thds_guard(threads);

  it block_start = first;
  for (unsigned i = 0; i < (num_threads - 1); i++) {
    it block_end = block_start;
    std::advance(block_end, block_size - 1);
    std::future<ValueType>* pre_fut = i == 0 ? nullptr : &fut_arr.back();
    threads[i] = std::thread(ChunkProcess(), block_start, block_end, pre_fut,
                             &prom_arr[i]);
    block_start = block_end;
    ++block_start;

    fut_arr.push_back(prom_arr[i].get_future());
  }

  std::future<ValueType>* pre_fut = fut_arr.empty() ? nullptr : &fut_arr.back();
  it block_end = first;
  std::advance(block_end, length - 1);
  ChunkProcess()(block_start, block_end, pre_fut, nullptr);
}
}  // namespace playground::parallel
#endif
