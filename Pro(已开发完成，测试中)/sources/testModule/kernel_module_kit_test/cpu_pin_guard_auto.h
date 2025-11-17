#pragma once
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <atomic>

/***************************************************************************
 * CpuPinGuardAuto
 * 作用: 在作用域内临时把当前线程锁定到“当前CPU”上，减少跨CPU切换抖动。
 * 特性:
 *   - 构造时:
 *       * 记录原始 CPU 亲和性
 *       * 获取当前正在运行的 CPU，并仅允许在该 CPU 上运行
 *       * 加一道 seq_cst fence，标记“已固定”
 *   - 析构时:
 *       * 恢复原始 CPU 亲和性
 *       * 再加一道 fence，标记“解除固定”
 *   - pinned()/cpu_id() 可查询是否固定成功以及固定到的 CPU 号
 ***************************************************************************/
class CpuPinGuardAuto {
public:
    CpuPinGuardAuto()
        : m_pinned(false),
          m_cpu_id(-1)
    {
        // 1. 读取当前线程原亲和性，之后析构时要恢复
        CPU_ZERO(&m_old_mask);
        if (sched_getaffinity(0, sizeof(m_old_mask), &m_old_mask) != 0) {
            fprintf(stderr, "CpuPinGuardAuto: sched_getaffinity failed: %s\n",
                    strerror(errno));
            return;
        }

        // 2. 获取当前正在运行的CPU编号
        int cur_cpu = sched_getcpu();
        if (cur_cpu < 0) {
            // 理论上很少失败，失败了就放弃pin
            fprintf(stderr, "CpuPinGuardAuto: sched_getcpu failed: %s\n",
                    strerror(errno));
            return;
        }

        m_cpu_id = cur_cpu;

        // 3. 构建一个只包含这个CPU的mask
        cpu_set_t new_mask;
        CPU_ZERO(&new_mask);
        CPU_SET(cur_cpu, &new_mask);

        if (sched_setaffinity(0, sizeof(new_mask), &new_mask) != 0) {
            // 可能是权限/策略/该CPU不允许你固定
            fprintf(stderr,
                    "CpuPinGuardAuto: sched_setaffinity(cpu %d) failed: %s\n",
                    cur_cpu, strerror(errno));
            return;
        }

        // 4. 加一道最强序列栅栏，告诉编译器/CPU：
        //    “在这个点之后，假定我们已经固定在该CPU上执行”
        std::atomic_thread_fence(std::memory_order_seq_cst);

        m_pinned = true;
        printf("CpuPinGuardAuto: pinned to CPU %d\n", m_cpu_id);
    }

    ~CpuPinGuardAuto() {
        if (!m_pinned) {
            return;
        }

        // 恢复原亲和性
        if (sched_setaffinity(0, sizeof(m_old_mask), &m_old_mask) != 0) {
            fprintf(stderr,
                    "CpuPinGuardAuto: restore sched_setaffinity failed: %s\n",
                    strerror(errno));
        }

        // 再来一道fence，语义上把“固定结束”这个点标出来
        std::atomic_thread_fence(std::memory_order_seq_cst);
        printf("CpuPinGuardAuto: restored from CPU %d\n", m_cpu_id);
    }

    // 获取被锁定的CPU号（-1表示没锁成功）
    int cpu_id() const { return m_cpu_id; }

    bool pinned() const { return m_pinned; }

private:
    cpu_set_t m_old_mask;
    bool      m_pinned;
    int       m_cpu_id;
};
