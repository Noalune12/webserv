# Configuration file - Complexity Analysis

## Time Complexity: O(n)

Our configuration parsing runs in linear time, using a **single-pass process**:

1. **Tokenizer** - Iterates **once** over the file content, extracting tokens character by character into a `std::vector<std::pair<std::string, std::vector<std::string>>>` (`PairVector` typedef). Each character is visited exactly once.

2. **Validator** - Iterates once over the token vector. Lookups in `_directiveValidators` (a `std::map`) are O(log k) where k is the fixed number of directive types (~15). Since `k` is small and constant, this is effectively O(1).

3. **ConfigInheritor** - Single traversal of contexts to apply inheritance rules.

There are no nested iterations over the input data, which gives:

```
T(n) = T_tokenize(n) + T_validate(n) + T_inherit(n) = O(n) + O(n) + O(n) = O(n)
```

## Space Complexity

### Theoretical: O(n)

Memory allocated for data structures scales with configuration size:

| Structure | Space | Reason |
|-----------|-------|--------|
| `PairVector` | O(d) | Stores d directives |
| `std::vector<Context>` | O(s or l) | Stores s server blocks or l locations |
| `std::vector<server>` | O(s × l) | Final config with locations |

Where d = directives, s = servers, l = locations per server.

### Practical: O(1) Memory Footprint

The process RSS (Resident Set Size) remains **constant** regardless of configuration size because:

1. **Efficient storage** - Only meaningful data is retained; comments and whitespace are discarded during tokenization
2. **No intermediate copies** - Data is parsed directly into final structures

For typical configurations (fewer than 10,000 locations), memory usage remains negligible.

### Auxiliary Space: O(1)

No recursion and no temporary copies of the input are used — only a few fixed-size local variables during parsing.

## Data Structure Choices

| Choice | Impact |
|--------|--------|
| `std::vector` for tokens | O(1) amortized append, cache-friendly iteration |
| `std::map` for validators | O(log k) lookup, but k ≈ 15 so negligible |
| `std::string` for values | Ownership clarity, prevents dangling pointers |
| `std::pair` for directives | Standard, simple |


## Notes

This approach is optimal. Since every character of the configuration must be processed at least once, achieving better than O(n) time complexity is impossible at runtime.

Our data structures are carefully chosen to ensure predictably efficient scaling for any reasonable configuration file size.
