/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef FATAL_INCLUDE_fatal_type_tuple_h
#define FATAL_INCLUDE_fatal_type_tuple_h

#include <fatal/container/tuple_tags.h>
#include <fatal/type/list.h>
#include <fatal/type/pair.h>
#include <fatal/type/tag.h>
#include <fatal/type/traits.h>

#include <tuple>
#include <type_traits>
#include <utility>

namespace fatal {

///////////
// tuple //
///////////

template <typename... Args>
struct tuple {
  using map = type_map<Args...>;
  using tags = tuple_tags<type_get_first<Args>...>;
  using values = type_list<type_get_second<Args>...>;
  using type = typename values::template apply<std::tuple>;

  template <typename TTag>
  using type_of = typename tags::template type_of<TTag, type>;

  template <std::size_t Index>
  using type_at = typename std::tuple_element<Index, type>;

  template <
    typename... UArgs,
    typename = safe_overload_t<tuple, UArgs...>
  >
  explicit constexpr tuple(UArgs &&...args):
    data_(std::forward<UArgs>(args)...)
  {}

  template <typename TTag>
  constexpr fast_pass<type_of<TTag>> get() const {
    return tags::template get<TTag>(data_);
  }

  template <typename TTag>
  type_of<TTag> &get() { return tags::template get<TTag>(data_); }

  template <std::size_t Index>
  constexpr fast_pass<type_at<Index>> at() const {
    return std::get<Index>(data_);
  }

  template <std::size_t Index>
  type_at<Index> &at() { return std::get<Index>(data_); }

  constexpr fast_pass<type> data() const { return data_; }
  type &data() { return data_; }

  // TODO: DOCUMENT AND TEST
  template <template <typename...> class TTransform>
  using apply = fatal::apply<TTransform, Args...>;

  // TODO: DOCUMENT AND TEST
  template <
    template <typename...> class TTypeTransform = identity_transform,
    template <typename...> class TTagTransform = identity_transform
  >
  using transform = typename map::template transform<
    TTypeTransform, TTagTransform
  >::contents::template apply<fatal::tuple>;

  // TODO: DOCUMENT AND TEST
  template <
    typename TTag,
    template <typename...> class TTypeTransform = identity_transform,
    template <typename...> class TTagTransform = identity_transform
  >
  using transform_at = typename map::template transform_at<
    TTag, TTypeTransform, TTagTransform
  >::template apply<fatal::tuple>;

  // TODO: DOCUMENT AND TEST
  template <typename... TPairs>
  using push_front = typename tags::list::template push_front<TPairs...>
    ::template apply<fatal::tuple>;

  // TODO: DOCUMENT AND TEST
  template <typename... TPairs>
  using push_back = typename tags::list::template push_back<TPairs...>
    ::template apply<fatal::tuple>;

  template <
    template <typename...> class TPredicate, typename V, typename... VArgs
  >
  constexpr std::size_t foreach_if(V &&visitor, VArgs &&...args) const {
    return tags::template foreach_if<TPredicate>(
      data_, std::forward<V>(visitor), std::forward<VArgs>(args)...
    );
  }

  template <
    template <typename...> class TPredicate, typename V, typename... VArgs
  >
  std::size_t foreach_if(V &&visitor, VArgs &&...args) {
    return tags::template foreach_if<TPredicate>(
      data_, std::forward<V>(visitor), std::forward<VArgs>(args)...
    );
  }

  template <typename V, typename... VArgs>
  constexpr bool foreach(V &&visitor, VArgs &&...args) const {
    return tags::foreach(
      data_, std::forward<V>(visitor), std::forward<VArgs>(args)...
    );
  }

  template <typename V, typename... VArgs>
  bool foreach(V &&visitor, VArgs &&...args) {
    return tags::foreach(
      data_, std::forward<V>(visitor), std::forward<VArgs>(args)...
    );
  }

  // TODO: TEST
  template <typename... UArgs>
  bool operator ==(tuple<UArgs...> const &rhs) const {
    return data_ == rhs.data();
  }

  // TODO: TEST
  template <typename... UArgs>
  bool operator !=(tuple<UArgs...> const &rhs) const {
    return !(*this == rhs);
  }

private:
  type data_;
};

////////////////
// tuple_from //
////////////////

// TODO: DOCUMENT AND TEST
template <typename... Args>
class tuple_from {
  template <
    template <typename...> class TTagTransform,
    template <typename...> class TTypeTransform
  >
  class impl {
    template <typename T>
    using pair = type_pair<
      fatal::apply<TTagTransform, T>,
      fatal::apply<TTypeTransform, T>
    >;

  public:
    template <typename... UArgs>
    using args = tuple<pair<UArgs>...>;

    template <typename TList>
    using list = typename TList::template apply<tuple, pair>;

    template <typename TMap>
    using map = typename TMap::template apply<
      tuple, TTypeTransform, TTagTransform
    >;
  };

public:
  template <
    template <typename...> class TTypeTransform = identity_transform,
    template <typename...> class TTagTransform = get_member_type::tag
  >
  using args = typename impl<TTagTransform, TTypeTransform>
    ::template args<Args...>;

  template <
    template <typename...> class TTypeTransform = identity_transform,
    template <typename...> class TTagTransform = get_member_type::tag
  >
  using list = fatal::apply<
    impl<TTagTransform, TTypeTransform>::template list,
    Args...
  >;

  template <
    template <typename...> class TTypeTransform = identity_transform,
    template <typename...> class TTagTransform = identity_transform
  >
  using map = fatal::apply<
    impl<TTagTransform, TTypeTransform>::template map,
    Args...
  >;
};

/////////////////
// build_tuple //
/////////////////

namespace detail {
namespace tuple_impl {

template <typename... Args>
class builder {
  using args = type_list<Args...>;

  using tags = typename args::template unzip<2, 0>;
  using types = typename args::template unzip<2, 1>;

  static_assert(tags::size == types::size, "not all tags map to a type");

public:
  using type = typename tags::template combine<type_pair>::template list<types>
    ::template apply<tuple>;
};

} // namespace tuple_impl {
} // namespace detail {

template <typename... Args>
using build_tuple = typename detail::tuple_impl::builder<
  Args...
>::type;

////////////////
// make_tuple //
////////////////

template <typename... TTags, typename... Args>
constexpr auto make_tuple(Args &&...args)
  -> tuple<type_pair<TTags, typename std::decay<Args>::type>...>
{
  return tuple<type_pair<TTags, typename std::decay<Args>::type>...>(
    std::forward<Args>(args)...
  );
}

template <typename... TTags, typename... Args>
constexpr auto make_tuple(std::tuple<Args...> tuple)
  -> fatal::tuple<type_pair<TTags, Args>...>
{
  return fatal::tuple<type_pair<TTags, Args>...>(std::move(tuple));
}

} // namespace fatal {

#endif // FATAL_INCLUDE_fatal_type_tuple_h