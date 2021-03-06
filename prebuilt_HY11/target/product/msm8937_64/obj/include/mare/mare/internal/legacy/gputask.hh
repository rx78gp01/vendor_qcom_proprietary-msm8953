// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013-2015 Qualcomm Technologies, Inc.
// All rights reserved.
// Confidential and Proprietary – Qualcomm Technologies, Inc.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#pragma once

#ifdef MARE_HAVE_OPENCL

#include <mare/internal/legacy/group.hh>
#include <mare/pfor_each.hh>

namespace mare {

namespace internal {

template<size_t Dims, typename Body, typename T>
struct ndrange_task_creator;

template<size_t Dims, typename Body>
struct ndrange_task_creator<Dims, Body, std::false_type>
{
  static task_shared_ptr create(const mare::range<Dims>& r,
                                                   Body&& body)
  {
    auto t = create_task([=]() {
               mare::pfor_each(r, body);
             });
    return t;
  }

  static task_shared_ptr create(const mare::range<Dims>& r,
                                                   const mare::range<Dims>& l,
                                                   Body&& body)
  {
    MARE_UNUSED(l);
    return create(r, std::forward<Body>(body));
  }
};

extern std::vector<::mare::internal::legacy::device_ptr> *s_dev_ptrs;

template<size_t Dims, typename Body>
struct ndrange_task_creator<Dims, Body, std::true_type>
{
  static task_shared_ptr create(const mare::range<Dims>& global_range,
                         const mare::range<Dims>& local_range,
                         Body&& body)
  {

    typedef ::mare::internal::legacy::body_wrapper<Body> wrapper;

    MARE_INTERNAL_ASSERT(!internal::s_dev_ptrs->empty(),
                         "No GPU devices found on the platform");
    for(auto i : global_range.stride())
      MARE_API_ASSERT( (i== 1), "GPU global ranges must be unit stride");
    for(auto i : local_range.stride())
      MARE_API_ASSERT( (i== 1), "GPU local ranges must be unit stride");
    internal::cldevice *d_ptr = internal::c_ptr(internal::s_dev_ptrs->at(0));
    MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
    internal::task* t = wrapper::create_task((*internal::s_dev_ptrs)[0],
                                             global_range,
                                             local_range,
                                             std::forward<Body>(body));

    return task_shared_ptr(t, task_shared_ptr::ref_policy::no_initial_ref);
  }
};

namespace legacy {

template<size_t Dims, typename Body>
inline ::mare::internal::task_shared_ptr create_ndrange_task(const range<Dims>& global_range,
                                                      const range<Dims>& local_range,
                                                      Body&& body)
{
  return internal::ndrange_task_creator<Dims, Body, typename std::is_base_of<
                                                    ::mare::internal::legacy::body_with_attrs_base_gpu,
                                                    Body>::type>::
           create(global_range, local_range, std::forward<Body>(body));
}

template<size_t Dims, typename Body>
inline ::mare::internal::task_shared_ptr create_ndrange_task(const range<Dims>& global_range,
                                                              Body&& body)
{
  range<Dims> local_range;
  return create_ndrange_task(global_range, local_range,
                             std::forward<Body>(body));
}

template<typename Body>
inline ::mare::internal::task_shared_ptr create_ndrange_task(size_t first, size_t last, Body&& body)
{

  mare::range<1> r(first, last);
  return create_ndrange_task(r, std::forward<Body>(body));
}

};
};
};

#endif
