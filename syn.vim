au FileType cpp-parse call AddSyn()

function! AddSyn()
	set ft=cpp
	syn match HELPER /helper([^)]\+)/
	syn match RULE /rule([^)]\+)/
	syn match RULE /rrule([^)]\+)/
	hi link HELPER Constant
	hi link RULE String
endfunction
