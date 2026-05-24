module bindc_helper_bc
    use, intrinsic :: iso_c_binding
    implicit none
contains

    function string_len(cstr)
        integer :: string_len
        character(kind=c_char,len=1), dimension(*) :: cstr

        string_len = 1
        do while(cstr(string_len) /= c_null_char)
            string_len = string_len + 1
        end do
        string_len = string_len - 1
    end function

    subroutine string_copy_c_f(c_string, f_string)
        character(len=1,kind=c_char), dimension(*), intent(in) :: c_string
        character(len=*), intent(out) :: f_string
        integer :: i, chlen

        i = 1
        chlen = len(f_string)
        do while(c_string(i)/=c_null_char .and. i<=chlen)
            f_string(i:i) = c_string(i)
            i = i + 1
        end do
        if (i<=chlen) f_string(i:) = ' '
    end subroutine

    subroutine string_copy_f_c(f_string, c_string)
        character(len=*), intent(in) :: f_string
        character(len=1,kind=c_char), dimension(*), intent(out) :: c_string(*)
        integer :: i, chlen

        i = 1
        chlen = len(f_string)
        do while(c_string(i)/=c_null_char .and. i<=chlen)
            c_string(i) = f_string(i:i)
            i = i + 1
        end do
    end subroutine
end module

