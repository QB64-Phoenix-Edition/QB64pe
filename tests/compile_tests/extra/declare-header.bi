
DECLARE LIBRARY "./header-dir/fastmath-extra"
    FUNCTION Fast_Sqrt& (BYVAL val AS LONG)
    FUNCTION Max_Long& ALIAS "Max<int>" (BYVAL x AS LONG, BYVAL y AS LONG)
    FUNCTION Max_Single! ALIAS "Max<float>" (BYVAL x AS SINGLE, BYVAL y AS SINGLE)
END DECLARE
