global invalid_tlb

section .text
invalid_tlb:
  push rbp 
  mov rbp, rsp 

  test rdi, 0xFFF
  jnz invalid_tlb_error

  invlpg [rdi]
  mfence 

  mov rsp, rbp 
  pop rbp 
  xor rax, rax 
  ret 
invalid_tlb_error:
  mov rsp, rbp
  pop rbp 
  mov rax, 1
  ret
