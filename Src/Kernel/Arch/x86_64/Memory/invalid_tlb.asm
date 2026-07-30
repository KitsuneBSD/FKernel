global arch_invlpg

section .text
arch_invlpg:
  push rbp 
  mov rbp, rsp 

  test rdi, 0xFFF
  jnz arch_invlpg_error

  invlpg [rdi]


  mov rsp, rbp 
  pop rbp 
  xor rax, rax 
  ret 
arch_invlpg_error:
  mov rsp, rbp
  pop rbp 
  mov rax, 1
  ret
