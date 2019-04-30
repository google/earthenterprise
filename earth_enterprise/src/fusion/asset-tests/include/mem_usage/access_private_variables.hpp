#ifndef _MEM_USAGE_ACCESS_PRIVATE_VARIABLES_HPP
#define _MEM_USAGE_ACCESS_PRIVATE_VARIABLES_HPP


namespace mem_usage {

namespace access_private_variables {

// From <https://gist.github.com/dabrahams/1528856>:

// This is a rewrite and analysis of the technique in this article:
// http://bloglitb.blogspot.com/2010/07/access-to-private-members-thats-easy.html

// ------- Framework -------
// The little library required to work this magic

// Generate a static data member of type Tag::type in which to store
// the address of a private member.  It is crucial that Tag does not
// depend on the /value/ of the the stored address in any way so that
// we can access it from ordinary code without directly touching
// private data.
template <class Tag>
struct stowed
{
     static typename Tag::type value;
}; 
template <class Tag> 
typename Tag::type stowed<Tag>::value;

// Generate a static data member whose constructor initializes
// stowed<Tag>::value.  This type will only be named in an explicit
// instantiation, where it is legal to pass the address of a private
// member.
template <class Tag, typename Tag::type x>
struct stow_private
{
     stow_private() { stowed<Tag>::value = x; }
     static stow_private instance;
};
template <class Tag, typename Tag::type x> 
stow_private<Tag,x> stow_private<Tag,x>::instance;


// // ------- Usage -------
// // A demonstration of how to use the library, with explanation

// struct A
// {
//      A() : x("proof!") {}
// private:
//      char const* x;
// };

// // A tag type for A::x.  Each distinct private member you need to
// // access should have its own tag.  Each tag should contain a
// // nested ::type that is the corresponding pointer-to-member type.
// struct A_x { typedef char const*(A::*type); };

// // Explicit instantiation; the only place where it is legal to pass
// // the address of a private member.  Generates the static ::instance
// // that in turn initializes stowed<Tag>::value.
// template class stow_private<A_x,&A::x>;

// int main()
// {
//         A a;

//         // Use the stowed private member pointer
//         std::cout << a.*stowed<A_x>::value << std::endl;
// };

}

}


#endif // _MEM_USAGE_ACCESS_PRIVATE_VARIABLES_HPP
