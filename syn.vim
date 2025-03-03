au FileType cpp-parse call AddSyn()

function! AddSyn()
	set ft=cpp
	syn match HELPER /helper([^)]\+)/
	syn match RULE /rule([^)]\+)/
	syn match RULE /rrule([^)]\+)/
	syn match RULE /frule([^)]\+)/
	syn match RULE /prule([^)]\+)/
	hi link HELPER Constant
	hi link RULE String
endfunction
