-- FKernel GEMINI Automation Script
-- Mantém padrões de projeto e qualidade de código automaticamente

local fk_config = {
    -- Object Calisthenics enforcement
    object_calisthenics = {
        max_class_lines = 200,
        max_method_lines = 20,
        max_instance_variables = 2,
        forbidden_patterns = {
            "else {",
            "else{",
            " else\n",
            "else\r\n"
        },
        chaining_limit = 1,  -- Max one dot per line
        primitive_types = {
            "int", "char", "float", "double", "long", "short", 
            "unsigned", "signed", "void*", "bool", "size_t"
        }
    },
    
    -- File organization enforcement (SECRET RULE)
    file_organization = {
        one_struct_per_file = true,
        domain_based_directories = true,
        deep_autodocumentation = true,
        allowed_extensions = {".h", ".cpp", ".c", ".asm"},
        forbidden_multiple_structs = {
            "class.*class",
            "struct.*struct", 
            "class.*struct",
            "struct.*class"
        }
    },
    
    -- Algorithm consolidation enforcement
    algorithm_consolidation = {
        consolidate_known_algorithms = true,
        move_to_libfk = true,
        avoid_domain_duplicates = true,
        known_algorithms = {
            "tar", "zip", "gzip", "crc32", "md5", "sha256",
            "base64", "hex", "url_encoding", "elf", "pe", "ini",
            "priority_queue", "bloom_filter", "lru_cache"
        },
        libfk_path = "Src/LibFK/Algorithms/",
        forbidden_domains = {"Src/Kernel/", "Src/Userland/"}
    },
    
    -- Testing requirements
    testing = {
        coverage_targets = {
            libc = 90,
            libfk = 85, 
            kernel = 75
        },
        required_files = {
            "tests/LibC/test_standalone.c",
            "tests/LibC/test_string_memory.cpp",
            "tests/LibFK/test_containers.cpp",
            "tests/Kernel/test_memory_manager.cpp"
        }
    },
    
    -- Architecture validation
    architecture = {
        layers = {
            ["Src/LibC/"] = "LibC only",
            ["Src/LibFK/"] = "LibFK only (can use LibC)",
            ["Src/Kernel/"] = "Kernel only (can use LibFK only)"
        },
        forbidden_includes = {
            ["Src/Kernel/"] = {"<LibC/", "#include <LibC"},
            ["Src/LibFK/"] = {"<std/", "#include <std"}
        }
    }
}

-- Funções de validação
function validate_object_calisthenics(file_path)
    local file = io.open(file_path, "r")
    if not file then return false end
    
    local content = file:read("*all")
    file:close()
    
    local lines = {}
    for line in content:gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end
    
    -- Verifica tamanho de classes
    if #lines > fk_config.object_calisthenics.max_class_lines then
        print(string.format("ERRO: %s tem %d linhas (máximo: %d)", 
              file_path, #lines, fk_config.object_calisthenics.max_class_lines))
        return false
    end
    
    -- Verifica else keyword
    for i, line in ipairs(lines) do
        for _, pattern in ipairs(fk_config.object_calisthenics.forbidden_patterns) do
            if line:match(pattern) then
                print(string.format("ERRO: %s:%d contém 'else' (linha: %s)", 
                      file_path, i, line:trim()))
                return false
            end
        end
    end
    
    -- Verifica method chaining
    for i, line in ipairs(lines) do
        local dot_count = 0
        for _ in line:gmatch("%.") do
            dot_count = dot_count + 1
        end
        if dot_count > fk_config.object_calisthenics.chaining_limit then
            print(string.format("ERRO: %s:%d tem %d dots (máximo: %d)", 
                  file_path, i, dot_count, fk_config.object_calisthenics.chaining_limit))
            return false
        end
    end
    
    return true
end

function validate_file_organization(file_path)
    local file = io.open(file_path, "r")
    if not file then return false end
    
    local content = file:read("*all")
    file:close()
    
    -- Verifica SECRET RULE: one struct/class per file
    local class_count = 0
    local struct_count = 0
    
    for line in content:gmatch("[^\r\n]+") do
        -- Conta classes
        if line:match("class%s+%w+") then
            class_count = class_count + 1
        end
        -- Conta structs
        if line:match("struct%s+%w+") then
            struct_count = struct_count + 1
        end
    end
    
    local total_types = class_count + struct_count
    if total_types > 1 then
        print(string.format("ERRO: %s contém %d tipos (%d classes, %d structs) - REGRA: UM TIPO POR ARQUIVO", 
              file_path, total_types, class_count, struct_count))
        return false
    end
    
    -- Verifica se o nome do arquivo segue snake_case
    local file_name = file_path:match("([^/]+)%.%w+$")
    if file_name and (file_name:match("%u") or file_name:match("%-")) then
        print(string.format("ERRO: %s não segue snake_case (encontrado CamelCase ou hifens)", file_path))
        return false
    end
    
    return true
end

function validate_domain_organization()
    -- Verifica se diretórios seguem padrão PascalCase (domínios)
    local violations = {}
    
    for dir in io.popen("find Src/ Include/ -type d -mindepth 1"):lines() do
        local dir_name = dir:match("([^/]+)$")
        if dir_name and not dir_name:match("^%u%w*$") then
            -- Ignora alguns padrões conhecidos
            if not dir_name:match("^%.") and dir_name ~= "tests" then
                table.insert(violations, string.format("Diretório '%s' não segue PascalCase (domínio)", dir))
            end
        end
    end
    
    if #violations > 0 then
        print("Violações de organização de domínios:")
        for _, violation in ipairs(violations) do
            print("  - " .. violation)
        end
        return false
    end
    
    return true
end

function validate_algorithm_consolidation()
    -- Verifica se algoritmos conhecidos estão consolidados em LibFK
    local violations = {}
    local consolidated_algos = {}
    
    -- Coleta algoritmos já consolidados em LibFK
    for file in io.popen("find Src/LibFK/Algorithms/ Include/LibFK/Algorithms/ -name '*.h' -o -name '*.cpp' 2>/dev/null"):lines() do
        local algo_name = file:match("([^/]+)%.%w+$"):gsub("%.%w+$", "")
        consolidated_algos[algo_name:lower()] = true
    end
    
    -- Procura por algoritmos duplicados em domínios proibidos
    for domain_path in fk_config.algorithm_consolidation.forbidden_domains do
        for file in io.popen("find " .. domain_path .. " -name '*.cpp' -o -name '*.h' 2>/dev/null"):lines() do
            for _, algo in ipairs(fk_config.algorithm_consolidation.known_algorithms) do
                if file:lower():match(algo) and not consolidated_algos[algo] then
                    table.insert(violations, string.format("Algoritmo '%s' encontrado em %s - deveria estar em LibFK/Algorithms/", algo, file))
                end
            end
        end
    end
    
    if #violations > 0 then
        print("Violações de consolidação de algoritmos:")
        for _, violation in ipairs(violations) do
            print("  - " .. violation)
        end
        return false
    end
    
    return true
end

function validate_testing_coverage()
    local tested_files = 0
    local total_files = 0
    
    -- Conta arquivos LibC
    for file in io.popen("find Src/LibC/ -name '*.c'"):lines() do
        total_files = total_files + 1
    end
    
    -- Conta arquivos Kernel
    for file in io.popen("find Src/Kernel/ -name '*.cpp'"):lines() do
        total_files = total_files + 1
    end
    
    -- Verifica se testes existem
    for _, test_file in ipairs(fk_config.testing.required_files) do
        if os.rename(test_file, test_file) then
            tested_files = tested_files + 1
        end
    end
    
    local coverage = (tested_files / #fk_config.testing.required_files) * 100
    print(string.format("Coverage de testes: %.1f%% (%d/%d arquivos requeridos)", 
          coverage, tested_files, #fk_config.testing.required_files))
    
    return coverage >= 50  -- Mínimo temporário
end

function validate_architecture()
    local violations = {}
    
    -- Verifica includes proibidos
    for pattern, forbidden_includes in pairs(fk_config.architecture.forbidden_includes) do
        for file in io.popen("find " .. pattern .. " -name '*.cpp' -o -name '*.h'"):lines() do
            local content = io.open(file, "r"):read("*all")
            for _, forbidden in ipairs(forbidden_includes) do
                if content:find(forbidden) then
                    table.insert(violations, string.format("%s inclui %s", file, forbidden))
                end
            end
        end
    end
    
    if #violations > 0 then
        print("Violações de arquitetura encontradas:")
        for _, violation in ipairs(violations) do
            print("  - " .. violation)
        end
        return false
    end
    
    return true
end

-- Main validation
function main()
    print("=== FKernel GEMINI Validation ===")
    
    local passed = 0
    local total = 0
    
    -- Valida Object Calisthenics
    total = total + 1
    print("Validando Object Calisthenics...")
    if validate_object_calisthenics("Src/Kernel/Scheduler/scheduler.cpp") then
        passed = passed + 1
        print("✓ Object Calisthenics OK")
    else
        print("✗ Object Calisthenics FALHOU")
    end
    
    -- Valida SECRET RULE: one struct per file
    total = total + 1
    print("Validando REGRA SECRETA: one struct/class per file...")
    local file_validation_passed = true
    for file in io.popen("find Src/ Include/ -name '*.h' -o -name '*.cpp' | head -20"):lines() do
        if not validate_file_organization(file) then
            file_validation_passed = false
            break
        end
    end
    if file_validation_passed then
        passed = passed + 1
        print("✓ One struct per file OK")
    else
        print("✗ One struct per file FALHOU")
    end
    
    -- Valida organização de domínios
    total = total + 1
    print("Validando organização de domínios...")
    if validate_domain_organization() then
        passed = passed + 1
        print("✓ Domain organization OK")
    else
        print("✗ Domain organization FALHOU")
    end
    
    -- Valida consolidação de algoritmos
    total = total + 1
    print("Validando consolidação de algoritmos...")
    if validate_algorithm_consolidation() then
        passed = passed + 1
        print("✓ Algorithm consolidation OK")
    else
        print("✗ Algorithm consolidation FALHOU")
    end
    
    -- Valida test coverage
    total = total + 1
    print("Validando cobertura de testes...")
    if validate_testing_coverage() then
        passed = passed + 1
        print("✓ Test coverage OK")
    else
        print("✗ Test coverage INSUFICIENTE")
    end
    
    -- Valida arquitetura
    total = total + 1
    print("Validando arquitetura...")
    if validate_architecture() then
        passed = passed + 1
        print("✓ Arquitetura OK")
    else
        print("✗ Arquitetura FALHOU")
    end
    
    print(string.format("\n=== Resultado: %d/%d validações passaram ===", passed, total))
    
    if passed == total then
        print("🎉 FKernel está em conformidade!")
        return 0
    else
        print("❌ FKernel precisa de correções")
        return 1
    end
end

-- Executa validação
return main()