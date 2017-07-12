#include <tuple>
#include <iterator>
#include <type_traits>

namespace pie {

namespace traits {
template <typename T>
struct is_pair : std::false_type { };

template <typename T, typename U>
struct is_pair<std::pair<T, U>> : std::true_type { };

template <typename T>
constexpr bool is_pair_v = is_pair<T>::value;

template <typename, typename = void>
struct is_mapping : std::false_type { };

template <typename Container>
struct is_mapping<Container, std::enable_if_t<
    is_pair_v<typename std::iterator_traits<
                typename Container::iterator>::value_type>>> : 
        std::true_type { };

template <typename T>
constexpr bool is_mapping_v = is_mapping<T>::value;

template <typename, typename = void>
struct is_sequence : std::false_type { };

template <typename Container>
struct is_sequence<Container, std::enable_if_t<
    !is_pair_v<typename std::iterator_traits<
                typename Container::iterator>::value_type>>> : 
        std::true_type { };

template <typename T>
constexpr bool is_sequence_v = is_sequence<T>::value;

template <typename>
struct is_string : std::false_type { };

template <>
struct is_string<std::string> : std::true_type { };

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

template <typename>
struct is_tuple : std::false_type { };

template <typename... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

template <typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

} // namespace traits

namespace detail {

template <typename> struct parse_object_format { };

template <>
struct parse_object_format<char> { static constexpr char value = 'i'; };

template <>
struct parse_object_format<unsigned char> { static constexpr char value = 'I'; };

template <>
struct parse_object_format<short> { static constexpr char value = 'i'; };

template <>
struct parse_object_format<unsigned short> { static constexpr char value = 'I'; };

template <>
struct parse_object_format<int> { static constexpr char value = 'i'; };

template <>
struct parse_object_format<unsigned int> { static constexpr char value = 'I'; };

template <>
struct parse_object_format<long> { static constexpr char value = 'i'; };

template <>
struct parse_object_format<unsigned long> { static constexpr char value = 'I'; };

template <>
struct parse_object_format<float> { static constexpr char value = 'f'; };

template <>
struct parse_object_format<double> { static constexpr char value = 'd'; };

template <>
struct parse_object_format<char*> { static constexpr char value = 's'; };

template <>
struct parse_object_format<const char*> { static constexpr char value = 's'; };

template <size_t I, typename Tuple>
struct set_tuple_helper {
    static void set(PyObject* o, Tuple t) {
        PyTuple_SetItem(o, I, parse_object(std::get<I>(t)));
        set_tuple_helper<I - 1, Tuple>::set(o, t);
    }
};

template <typename Tuple>
struct set_tuple_helper<0, Tuple> {
    static void set(PyObject* o, Tuple t) {
        PyTuple_SetItem(o, 0, parse_object(std::get<0>(t)));
    }
};

template <typename T, typename = void>
struct parse_object_helper {
    static PyObject* parse(T&& t) {
        char format[2] = {0};
        format[0] = parse_object_format<std::decay_t<T>>::value;
        return Py_BuildValue(format, t);
    }
};

template <typename Tuple>
struct parse_object_helper<Tuple, std::enable_if_t<traits::is_tuple_v<std::decay_t<Tuple>>>> {
    static PyObject* parse(Tuple&& t) {
        constexpr size_t n = std::tuple_size<Tuple>::value;
        PyObject* o = PyTuple_New(n);
        set_tuple_helper<n - 1, Tuple>::set(o, t);
        return o;
    }
};

template <typename String>
struct parse_object_helper<String, std::enable_if_t<traits::is_string_v<std::decay_t<String>>>> {
    static PyObject* parse(String&& s) {
        char format[2] = {0};
        format[0] = parse_object_format<const char*>::value;
        return Py_BuildValue(format, s.c_str());
    }
};

template <typename Container>
struct parse_object_helper<Container, std::enable_if_t<traits::is_mapping_v<Container>>> {
    static PyObject* parse(Container c) {
        PyObject* o = PyDict_New();
        for (auto& e: c) {
            PyObject* key = parse_object(e.first);
            PyObject* value = parse_object(e.second);
            Py_INCREF(key);
            Py_INCREF(value);
            PyDict_SetItem(o, key, value);
        }
        return o;
    }
};

template <typename Container>
struct parse_object_helper<Container, std::enable_if_t<traits::is_sequence_v<Container>>> {
    static PyObject* parse(Container c) {
        PyObject* o = PyList_New(0);
        for (auto& e: c) {
            PyList_Append(o, parse_object(e));
        }
        return o;
    }
};

} // namespace detail

template <typename T>
PyObject* parse_object(T&& t) {
    return detail::parse_object_helper<T>::parse(std::forward<T>(t));
}

} // namespace pie
