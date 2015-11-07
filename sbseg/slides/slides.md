% Implementação eficiente em software da função Lyra2 em arquiteturas modernas
% Guilherme P. Gonçalves$^{1}$; Diego F. Aranha[^1]
% Nov/2015

[^1]: Laboratório de Segurança e Criptografia (LASCA) -- UNICAMP

# Introdução - Esquemas modernos de hash de senhas

Forma extremamente comum de autenticação

Esquema de _hash_ deve ser **seguro**

- Resistência ao cálculo de pré-imagem
- Proteção contra adversários...
    - ...com acesso direto ao banco de dados
    - ...executando ataques de busca exaustiva
    - ...usando _hardware_ dedicado

Ashley Madison: bcrypt :-) / md5 da senha "normalizada" :-(

# Introdução - Esquemas modernos de hash de senhas

Interface: $h = HASH(password, salt, T, M)$

Esquemas parametrizados:

- bcrypt (1999), parâmetro de tempo
- PBKDF2 (2000), parâmetro de tempo
- scrypt (2012), um parâmetro de tempo e espaço
- _Lyra2 (2014), parâmetros independentes de tempo e espaço_

**Como escolher os parâmetros?**

# O esquema de hash de senhas Lyra2

Finalista do Password Hashing Competition

Construção de esponja e função de compressão da BLAKE2b

Este trabalho:

- Implementação em _software_ do Lyra2 v2.5 sequencial
- Inconsistências na especificação/implementação de referência
- Desempenho cerca de 17% (SSE2) e 30% (AVX2) superior

# Lyra2 - A construção de esponja

\setbeamertemplate{caption}{\insertcaption}
![A construção de esponja](../img/sponge.png)

Generaliza funções de _hash_

Construção similar: _duplex_

No Lyra2: esponja suporta _absorb_, _squeeze_ e _duplex_ independentes

# Lyra2 - A função de compressão

Com $a, b, c, d$ inteiros de 64 bits, define-se $G(a, b, c, d)$:

\vspace{-0.75cm}
\begin{small}
\begin{align*}
\begin{split}
& a \leftarrow a + b \\
& d \leftarrow \left(d \oplus a \right) \ggg 32 \\
& c \leftarrow c + d \\
& b \leftarrow \left(b \oplus c \right) \ggg 24 \\
& a \leftarrow a + b \\
& d \leftarrow \left(d \oplus a \right) \ggg 16 \\
& c \leftarrow c + d \\
& b \leftarrow \left(b \oplus c \right) \ggg 63
\end{split}
\end{align*}
\end{small}
\vspace{-0.5cm}

Na BLAKE2b: Matriz de estado $4\times4$ de inteiros

$G_{//}(colunas) + G_{//}(diagonais)$ = _rodada_ = $f$

Estado da esponja = matriz de estado BLAKE2b linearizada

# Lyra2 - Descrição

Interface: $h = LYRA2(password, salt, T, R, C)$

Matriz de estado de $R \times C$ blocos de $b$ bits

Três etapas:

- _Bootstrapping_: inicializa a esponja, _absorbs_ das entradas
- _Setup_: inicializa a matriz de estado com _squeezes_ e _duplexes_
- _Wandering_: _duplex_ de células pseudoaleatórias, $T$ iterações

Finalmente: $h = squeeze()$

Tempo: $O((T + 1) \cdot R \cdot C)$

Espaço: $O(R \cdot C)$

# Implementação proposta

Disponível em: https://github.com/guilherme-pg/lyra2.git

Licença: MIT

Versões SSE2 e AVX2 escritas em C (padrão C99)

Fácil extensão para novos conjuntos de instruções

# Implementação proposta - AVX2

Conjunto de instruções recente: vetores 256 _bits_

Implementação nova da função de compressão

Matriz de estado BLAKE2b / estado da esponja Lyra2:

\vspace{-0.7cm}
\begin{align*}
\left(
\begin{matrix}
v_{0} & v_{1} & v_{2} & v_{3} \\
v_{4} & v_{5} & v_{6} & v_{7} \\
v_{8} & v_{9} & v_{10} & v_{11} \\
v_{12} & v_{13} & v_{14} & v_{15}
\end{matrix}
\right)
\end{align*}
\vspace{-0.5cm}

Rodada: $G_{//}(colunas)$, _diagonalize_, $G_{//}(colunas)$, _undiagonalize_

Semelhante à versão SSE2, mas 1 linha = 1 vetor

# Resultados experimentais

Tempos de execução normalizados (quanto menor, melhor)

\setbeamertemplate{caption}{\insertcaption}
![Resultados experimentais em um Intel Core i7-4770 (Haswell)](../img/linux.png)

Legenda:

- ref-gcc, ref-clang: implementação de referência, SSE2
- gcc, clang: implementação proposta, SSE2
- avx2-gcc, avx2-clang: implementação proposta, AVX2

# Trechos de código - Absorb

~~~{.C .numberLines}
static inline void
sponge_absorb(sponge_t *sponge, sponge_word_t *data,
              size_t databytes) {
    size_t rate = SPONGE_RATE_LENGTH;
    size_t datalenw = databytes / sizeof(sponge_word_t);
    assert(datalenw % rate == 0);
    while (datalenw) {
        for (unsigned int i = 0; i < rate; i++) {
            sponge->state[i] ^= data[i];
        }
        sponge_compress(sponge);
        data += rate;
        datalenw -= rate;
    }
    return;
}
~~~

# Trechos de código - Função G

~~~{.C .numberLines}
#define G(row1,row2,row3,row4)         \
  row1 = _mm256_add_epi64(row1, row2); \
  row4 = _mm256_xor_si256(row4, row1); \
  row4 = _mm256_roti_epi64(row4, -32); \
  row3 = _mm256_add_epi64(row3, row4); \
  row2 = _mm256_xor_si256(row2, row3); \
  row2 = _mm256_roti_epi64(row2, -24); \
  row1 = _mm256_add_epi64(row1, row2); \
  row4 = _mm256_xor_si256(row4, row1); \
  row4 = _mm256_roti_epi64(row4, -16); \
  row3 = _mm256_add_epi64(row3, row4); \
  row2 = _mm256_xor_si256(row2, row3); \
  row2 = _mm256_roti_epi64(row2, -63);
~~~
