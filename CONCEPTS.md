# Some internal concepts

## SEL4 Capabitilies

O FKernel possui as Capabitilies puxadas junto do IPC do SEL4, mas não existe forma leiga de se usar, é sempre uma dor de parto.

Então porque nao juntar o util ao agradavel, e expor o Pledge e o Unveil do OpenBSD enquanto por baixo usa os capabilities do SEL4. 

Isso significa que o sistema já tem um certo nível de hardening por padrão

## ZRAM / ZSWAP

O FKernel possui um gerenciamento de memória interessante porém limitado, para continuar progredindo uma hora ou outra precisaria delegar parte da memória ao SWAP que ajudaria o kernel a se manter sobre carga. Mas o swap é lento, pensando nisso uma solucao a parte seria adicionar suporte a compressão de memória como uma etapa anterior.
