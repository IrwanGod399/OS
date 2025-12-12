#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h> 

/* =========================================================
   KONFIGURASI SYSTEM CALL BUATAN
   PENTING: Pastikan angka 548 ini SAMA dengan yang ada 
   di file syscall_64.tbl kernel Anda.
   ========================================================= */
#define SYS_IRWAN_LOG 548

/* KONFIGURASI SCHEDULER */
#define JUMLAH_PROSES 3
#define QUANTUM_DETIK 3

/* Variabel Global (Untuk Scheduler) */
pid_t pids[JUMLAH_PROSES];
int current_idx = 0;

/* ---------------------------------------------------------
   FUNGSI ANAK (WORKER)
   --------------------------------------------------------- */
void do_work(int id) {
    long kernel_pid;
    int counter = 1;

    for(int i = 1;i < 30;i++) {
        // Ini system call standar (getpid)
        kernel_pid = syscall(SYS_getpid); 
		
        printf("  [User Program %d] (Kernel PID: %ld) sedang bekerja... Sekarang menghitung %d\n", id+1, kernel_pid, i);
        
        usleep(1000000); 
    }
}

/* ---------------------------------------------------------
   FUNGSI SCHEDULER (KERNEL SIMULATION)
   --------------------------------------------------------- */
// Fungsi ini dipanggil otomatis saat Timer meledak (SIGALRM)
void scheduler_handler(int signum) {
    printf("\n[SCHEDULER] Waktu Habis (Quantum %d detik)!\n", QUANTUM_DETIK);
    
    // 1. FREEZE proses yang sedang jalan (SIGSTOP)
    printf("[SCHEDULER] Membekukan PID %d...\n", pids[current_idx]);
    kill(pids[current_idx], SIGSTOP);

    // 2. GANTI giliran (Round Robin Algorithm)
    current_idx = (current_idx + 1) % JUMLAH_PROSES;

    // ---------------------------------------------------------
    // INTEGRASI SYSTEM CALL (MODIFIKASI UTAMA)
    // ---------------------------------------------------------
    // Melapor ke Kernel Log (dmesg) sebelum menjalankan proses
    printf("[SCHEDULER] Memanggil System Call %d ke Kernel...\n", SYS_IRWAN_LOG);
    
    long res = syscall(SYS_IRWAN_LOG, pids[current_idx]); // Memanggil Kernel
    
    if (res == 0) {
        printf("[SUCCESS] Kernel Log tercatat.\n");
    } else {
        // Error handling opsional, abaikan jika kernel belum support
        // printf("[ERROR] Gagal memanggil kernel.\n"); 
    }
    // ---------------------------------------------------------

    // 3. RESUME proses selanjutnya (SIGCONT)
    printf("[SCHEDULER] Mengaktifkan PID %d...\n", pids[current_idx]);
    printf("--------------------------------------------------\n\n");
    kill(pids[current_idx], SIGCONT);
}

int main() {
    printf("=== SIMULASI TIME-SHARING DENGAN MODIFIED SYSTEM CALL ===\n");
    printf("Parent PID (Scheduler): %d\n\n", getpid());

    // --- LANGKAH 1: MEMBUAT PROSES (FORK) ---
    for (int i = 0; i < JUMLAH_PROSES; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            do_work(i); // Area Anak
            exit(0);
        } else {
            pids[i] = pid; // Area Bapak
            kill(pid, SIGSTOP); // Stop anak segera
        }
    }

    // --- LANGKAH 2: SETUP SINYAL & TIMER ---
    signal(SIGALRM, scheduler_handler);

    struct itimerval timer;
    timer.it_interval.tv_sec = QUANTUM_DETIK;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = QUANTUM_DETIK;
    timer.it_value.tv_usec = 0;

    // --- LANGKAH 3: JALANKAN SISTEM ---
    printf("[INIT] Menjalankan Proses Pertama (PID: %d)...\n", pids[0]);
    
    // Panggil syscall juga untuk proses pertama kali jalan
    syscall(SYS_IRWAN_LOG, pids[0]);
    
    kill(pids[0], SIGCONT);
    setitimer(ITIMER_REAL, &timer, NULL);

    while(1) {
        pause(); 
    }

    return 0;
}



