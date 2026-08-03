# Some internal concepts

## SEL4 Capabitilies

o FKernel possui as Capabitilies puxadas junto do IPC do SEL4, mas não existe forma leiga de se usar, é sempre uma dor de parto.

Então porque nao juntar o util ao agradavel, e expor o Pledge e o Unveil do OpenBSD enquanto por baixo usa os capabilities do SEL4. 

Isso significa que o sistema já tem um certo nível de hardening por padrão
